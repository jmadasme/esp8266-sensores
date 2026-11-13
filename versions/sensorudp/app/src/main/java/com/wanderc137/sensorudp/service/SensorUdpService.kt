package com.wanderc137.sensorudp.service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.hardware.TriggerEvent
import android.hardware.TriggerEventListener
import android.net.wifi.WifiManager
import android.os.Build
import android.os.IBinder
import android.os.PowerManager
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat
import com.wanderc137.sensorudp.MainActivity
import com.wanderc137.sensorudp.R
import com.wanderc137.sensorudp.data.SensorRepository
import com.wanderc137.sensorudp.data.model.SensorInfo
import com.wanderc137.sensorudp.data.model.SensorSample
import com.wanderc137.sensorudp.network.UdpConfig
import com.wanderc137.sensorudp.network.UdpSender
import dagger.hilt.android.AndroidEntryPoint
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * Foreground service que registra todos los sensores, encola sus lecturas
 * y las transmite por UDP mediante [UdpSender].
 */
@AndroidEntryPoint
class SensorUdpService : Service() {

    @Inject
    lateinit var udpSender: UdpSender

    @Inject
    lateinit var stateHolder: ServiceStateHolder

    @Inject
    lateinit var sensorRepository: SensorRepository

    private lateinit var sensorManager: SensorManager
    private var wakeLock: PowerManager.WakeLock? = null
    private var wifiLock: WifiManager.WifiLock? = null

    private val registeredSensors = mutableListOf<Sensor>()
    private var notificationUpdateJob: Job? = null
    private val serviceScope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)

    @Volatile
    private var transmissionActive = false

    private val sensorListener = object : SensorEventListener {
        override fun onSensorChanged(event: SensorEvent) {
            val sensor = event.sensor ?: return
            udpSender.enqueue(
                SensorSample(
                    sensor = sensor.name,
                    type = sensor.type,
                    typeName = SensorInfo.typeName(sensor.type),
                    eventTimestamp = event.timestamp,
                    values = event.values.map { it }
                )
            )
        }

        override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) = Unit
    }

    private val triggerListener = object : TriggerEventListener() {
        override fun onTrigger(event: TriggerEvent) {
            val sensor = event.sensor ?: return
            udpSender.enqueue(
                SensorSample(
                    sensor = sensor.name,
                    type = sensor.type,
                    typeName = SensorInfo.typeName(sensor.type),
                    eventTimestamp = event.timestamp,
                    values = event.values.map { it }
                )
            )
        }
    }

    override fun onCreate() {
        super.onCreate()
        sensorManager = sensorRepository.sensorManager()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> {
                val ip = intent.getStringExtra(EXTRA_IP)
                val port = intent.getIntExtra(EXTRA_PORT, UdpConfig.DEFAULT_PORT)
                if (ip.isNullOrBlank()) {
                    stateHolder.update {
                        it.copy(lastError = getString(R.string.error_ip_requerida))
                    }
                    stopSelf()
                } else {
                    startTransmission(UdpConfig(ip, port))
                }
            }

            ACTION_UPDATE -> {
                val ip = intent.getStringExtra(EXTRA_IP)
                val port = intent.getIntExtra(EXTRA_PORT, UdpConfig.DEFAULT_PORT)
                if (!ip.isNullOrBlank() && transmissionActive) {
                    try {
                        udpSender.updateConfig(UdpConfig(ip, port))
                        stateHolder.update {
                            it.copy(targetIp = ip, targetPort = port, lastError = null)
                        }
                        updateNotification()
                    } catch (t: Throwable) {
                        stateHolder.update {
                            it.copy(lastError = t.message ?: t.javaClass.simpleName)
                        }
                    }
                }
            }

            ACTION_STOP -> stopSelf()
        }
        return START_NOT_STICKY
    }

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onTaskRemoved(rootIntent: Intent?) {
        // Plan 4.7: si el usuario cierra la app, se detiene la transmisión.
        stopSelf()
        super.onTaskRemoved(rootIntent)
    }

    override fun onDestroy() {
        stopTransmission()
        serviceScope.cancel()
        super.onDestroy()
    }

    private fun startTransmission(config: UdpConfig) {
        if (transmissionActive) {
            // Ya transmitiendo: aplicar la nueva dirección en caliente.
            try {
                udpSender.updateConfig(config)
                stateHolder.update { it.copy(targetIp = config.ip, targetPort = config.port) }
            } catch (t: Throwable) {
                stateHolder.update {
                    it.copy(lastError = t.message ?: t.javaClass.simpleName)
                }
            }
            return
        }

        startForegroundCompat()

        try {
            udpSender.start(config)
        } catch (t: Throwable) {
            stateHolder.update {
                it.copy(
                    isRunning = false,
                    lastError = getString(
                        R.string.error_socket,
                        t.message ?: t.javaClass.simpleName
                    )
                )
            }
            ServiceCompat.stopForeground(this, ServiceCompat.STOP_FOREGROUND_REMOVE)
            stopSelf()
            return
        }

        if (!acquireLocks()) {
            // I-004: sin locks no se garantiza la transmisión continua:
            // liberar lo adquirido parcialmente y detenerse limpiamente.
            releaseLocks()
            udpSender.stop()
            ServiceCompat.stopForeground(this, ServiceCompat.STOP_FOREGROUND_REMOVE)
            stopSelf()
            return
        }
        registerAllSensors()

        transmissionActive = true
        stateHolder.update {
            it.copy(
                isRunning = true,
                targetIp = config.ip,
                targetPort = config.port,
                packetsSent = 0,
                samplesSent = 0,
                droppedSamples = 0,
                activeSensors = registeredSensors.size,
                lastError = null
            )
        }
        startNotificationUpdates()
        updateNotification()
    }

    private fun stopTransmission() {
        val hadResources =
            transmissionActive || wakeLock != null || wifiLock != null

        transmissionActive = false
        notificationUpdateJob?.cancel()
        notificationUpdateJob = null

        if (hadResources) {
            unregisterAllSensors()
            udpSender.stop()
            releaseLocks()
        }

        stateHolder.update {
            it.copy(
                isRunning = false,
                activeSensors = 0,
                // I-005: `udpSender.stop()` limpia su `lastError` (cierre
                // voluntario del socket). Se conserva el último error real ya
                // reflejado en el estado para no perderlo.
                lastError = it.lastError
            )
        }
        ServiceCompat.stopForeground(this, ServiceCompat.STOP_FOREGROUND_REMOVE)
    }

    private fun startForegroundCompat() {
        val notification = buildNotification()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            ServiceCompat.startForeground(
                this,
                NOTIFICATION_ID,
                notification,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_DATA_SYNC
            )
        } else {
            startForeground(NOTIFICATION_ID, notification)
        }
    }

    /**
     * Registra TODOS los sensores detectados respetando su modo de reporte:
     * continuous → FASTEST, on-change/special → NORMAL, one-shot → trigger.
     */
    private fun registerAllSensors() {
        unregisterAllSensors()
        for (sensor in sensorManager.getSensorList(Sensor.TYPE_ALL)) {
            val registered = when (sensor.reportingMode) {
                Sensor.REPORTING_MODE_ONE_SHOT ->
                    sensorManager.requestTriggerSensor(triggerListener, sensor)

                Sensor.REPORTING_MODE_CONTINUOUS ->
                    sensorManager.registerListener(
                        sensorListener,
                        sensor,
                        SensorManager.SENSOR_DELAY_FASTEST
                    )

                else ->
                    sensorManager.registerListener(
                        sensorListener,
                        sensor,
                        SensorManager.SENSOR_DELAY_NORMAL
                    )
            }
            if (registered) {
                registeredSensors.add(sensor)
            }
        }
    }

    private fun unregisterAllSensors() {
        if (registeredSensors.isEmpty()) return
        sensorManager.unregisterListener(sensorListener)
        for (sensor in registeredSensors) {
            if (sensor.reportingMode == Sensor.REPORTING_MODE_ONE_SHOT) {
                sensorManager.cancelTriggerSensor(triggerListener, sensor)
            }
        }
        registeredSensors.clear()
    }

    /**
     * Adquiere WakeLock y WifiLock de forma independiente y robusta (I-004).
     * Si falla cualquiera de los dos, libera lo adquirido, reporta el error
     * y devuelve `false` para que el llamador detenga el servicio limpiamente.
     */
    @Suppress("DEPRECATION")
    private fun acquireLocks(): Boolean {
        if (wakeLock == null) {
            try {
                val powerManager = getSystemService(Context.POWER_SERVICE) as PowerManager
                wakeLock = powerManager
                    .newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, WAKE_LOCK_TAG)
                    .apply {
                        setReferenceCounted(false)
                        acquire()
                    }
            } catch (t: Throwable) {
                wakeLock = null
                stateHolder.update {
                    it.copy(
                        lastError = getString(
                            R.string.error_wakelock,
                            t.message ?: t.javaClass.simpleName
                        )
                    )
                }
                releaseLocks()
                return false
            }
        }
        if (wifiLock == null) {
            try {
                val wifiManager =
                    applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
                wifiLock = wifiManager
                    .createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, WIFI_LOCK_TAG)
                    .apply {
                        setReferenceCounted(false)
                        acquire()
                    }
            } catch (t: Throwable) {
                wifiLock = null
                stateHolder.update {
                    it.copy(
                        lastError = getString(
                            R.string.error_wifilock,
                            t.message ?: t.javaClass.simpleName
                        )
                    )
                }
                releaseLocks()
                return false
            }
        }
        return true
    }

    private fun releaseLocks() {
        wakeLock?.let { lock ->
            try {
                if (lock.isHeld) lock.release()
            } catch (_: Throwable) {
                // Un fallo al liberar no debe impedir liberar el resto.
            }
        }
        wakeLock = null
        wifiLock?.let { lock ->
            try {
                if (lock.isHeld) lock.release()
            } catch (_: Throwable) {
                // Idem.
            }
        }
        wifiLock = null
    }

    private fun startNotificationUpdates() {
        notificationUpdateJob?.cancel()
        notificationUpdateJob = serviceScope.launch {
            while (isActive) {
                delay(NOTIFICATION_REFRESH_MS)
                if (!transmissionActive) break
                refreshCounters()
            }
        }
    }

    private fun refreshCounters() {
        val counters = udpSender.counters()
        stateHolder.update {
            it.copy(
                packetsSent = counters.packetsSent,
                samplesSent = counters.samplesSent,
                droppedSamples = counters.droppedSamples,
                lastError = udpSender.lastError
            )
        }
        updateNotification()
    }

    private fun createNotificationChannel() {
        val manager = getSystemService(NotificationManager::class.java) ?: return
        val channel = NotificationChannel(
            CHANNEL_ID,
            getString(R.string.notification_channel_name),
            NotificationManager.IMPORTANCE_LOW
        ).apply {
            description = getString(R.string.notification_channel_description)
            setShowBadge(false)
        }
        manager.createNotificationChannel(channel)
    }

    private fun buildNotification(): Notification {
        val state = stateHolder.current()
        val openPendingIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        val stopPendingIntent = PendingIntent.getService(
            this,
            1,
            Intent(this, SensorUdpService::class.java).setAction(ACTION_STOP),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_notification)
            .setContentTitle(getString(R.string.notification_title))
            .setContentText(
                getString(
                    R.string.notification_text,
                    state.packetsSent,
                    state.samplesSent,
                    state.activeSensors
                )
            )
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setContentIntent(openPendingIntent)
            .addAction(0, getString(R.string.notification_action_stop), stopPendingIntent)
            .setForegroundServiceBehavior(NotificationCompat.FOREGROUND_SERVICE_IMMEDIATE)
            .build()
    }

    private fun updateNotification() {
        val manager = getSystemService(NotificationManager::class.java) ?: return
        manager.notify(NOTIFICATION_ID, buildNotification())
    }

    companion object {
        const val ACTION_START = "com.wanderc137.sensorudp.action.START"
        const val ACTION_STOP = "com.wanderc137.sensorudp.action.STOP"
        const val ACTION_UPDATE = "com.wanderc137.sensorudp.action.UPDATE"

        const val EXTRA_IP = "extra_ip"
        const val EXTRA_PORT = "extra_port"

        private const val CHANNEL_ID = "sensorudp_channel"
        private const val NOTIFICATION_ID = 5005

        /** Intervalo de refresco de la notificación (menos frecuente que el envío). */
        private const val NOTIFICATION_REFRESH_MS = 1000L

        private const val WAKE_LOCK_TAG = "SensorUDP::transmissionWakeLock"
        private const val WIFI_LOCK_TAG = "SensorUDP::transmissionWifiLock"

        fun startIntent(context: Context, ip: String, port: Int): Intent =
            Intent(context, SensorUdpService::class.java)
                .setAction(ACTION_START)
                .putExtra(EXTRA_IP, ip)
                .putExtra(EXTRA_PORT, port)

        fun updateIntent(context: Context, ip: String, port: Int): Intent =
            Intent(context, SensorUdpService::class.java)
                .setAction(ACTION_UPDATE)
                .putExtra(EXTRA_IP, ip)
                .putExtra(EXTRA_PORT, port)

        fun stopIntent(context: Context): Intent =
            Intent(context, SensorUdpService::class.java).setAction(ACTION_STOP)
    }
}

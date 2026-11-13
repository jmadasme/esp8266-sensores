package com.wanderc137.sensorudp.network

import android.os.Build
import com.google.gson.Gson
import com.wanderc137.sensorudp.data.model.SensorSample
import com.wanderc137.sensorudp.data.model.UdpBatch
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.util.concurrent.ArrayBlockingQueue
import java.util.concurrent.atomic.AtomicLong
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Envía muestras de sensores por UDP agrupadas en lotes JSON.
 *
 * - `enqueue()` lo llaman los callbacks de sensores (nunca bloquea).
 * - Un hilo dedicado consume la cola cada ~20 ms y envía un datagrama por lote.
 * - Si un lote excede el MTU útil, se fragmenta en varios datagramas con el mismo esquema.
 */
@Singleton
class UdpSender @Inject constructor(
    private val gson: Gson
) {

    data class Counters(
        val packetsSent: Long,
        val samplesSent: Long,
        val droppedSamples: Long
    )

    companion object {
        /** Intervalo objetivo entre lotes. */
        const val BATCH_INTERVAL_MS = 20L

        /** Payload máximo por datagrama para no fragmentar a nivel IP (~MTU típica). */
        const val MAX_DATAGRAM_PAYLOAD = 1400

        /** Capacidad de la cola; al llenarse se descarta la muestra más antigua. */
        const val MAX_QUEUE_SIZE = 16384
    }

    private val queue = ArrayBlockingQueue<SensorSample>(MAX_QUEUE_SIZE)
    private val packetsSent = AtomicLong(0)
    private val samplesSent = AtomicLong(0)
    private val droppedSamples = AtomicLong(0)

    @Volatile
    private var running = false

    @Volatile
    private var socket: DatagramSocket? = null

    @Volatile
    private var targetAddress: InetAddress? = null

    @Volatile
    private var targetPort: Int = UdpConfig.DEFAULT_PORT

    /** Último error de socket; se limpia al volver a enviar con éxito. */
    @Volatile
    var lastError: String? = null
        private set

    private var senderThread: Thread? = null
    private var sequence = 1L

    private val deviceName: String = Build.DEVICE ?: "android"
    private val modelName: String = Build.MODEL ?: "Android"

    /**
     * Abre el socket y arranca el hilo de envío.
     * @throws Exception si la dirección no se resuelve o el socket no puede abrirse.
     */
    @Synchronized
    fun start(config: UdpConfig) {
        stop()
        val address = InetAddress.getByName(config.ip.trim())
        val newSocket = DatagramSocket()
        socket = newSocket
        targetAddress = address
        targetPort = config.port
        sequence = 1
        lastError = null
        running = true
        senderThread = Thread({ sendLoop() }, "SensorUDP-sender").apply {
            // Más conservadora que MAX_PRIORITY para no arriesgar inanición
            // de otros hilos del proceso (observación del auditor).
            priority = Thread.NORM_PRIORITY + 1
            start()
        }
    }

    /**
     * Cambia IP/puerto en caliente sin detener el hilo de envío.
     *
     * [Synchronized] con [start]/[stop] para que `targetAddress`, `targetPort`
     * y `socket` sean siempre visibles/consistentes entre hilos (I-001).
     * @throws Exception si la dirección no se resuelve.
     */
    @Synchronized
    fun updateConfig(config: UdpConfig) {
        targetAddress = InetAddress.getByName(config.ip.trim())
        targetPort = config.port
        lastError = null
    }

    /** Encola una muestra sin bloquear. Si la cola está llena, descarta la más antigua. */
    fun enqueue(sample: SensorSample) {
        if (!running) return
        if (!queue.offer(sample)) {
            queue.poll()
            droppedSamples.incrementAndGet()
            queue.offer(sample)
        }
    }

    @Synchronized
    fun stop() {
        running = false
        senderThread?.interrupt()
        try {
            senderThread?.join(1000)
        } catch (_: InterruptedException) {
            Thread.currentThread().interrupt()
        }
        senderThread = null
        socket?.close()
        socket = null
        targetAddress = null
        queue.clear()
        // I-005: la parada es voluntaria; un `lastError` pendiente sería un
        // artefacto del cierre del socket (p. ej. "Socket closed"), no una
        // falla real de transmisión. Los errores reales ya se reflejan en la
        // UI mientras la transmisión está activa.
        lastError = null
    }

    fun counters(): Counters =
        Counters(packetsSent.get(), samplesSent.get(), droppedSamples.get())

    private fun sendLoop() {
        val batch = ArrayList<SensorSample>(512)
        while (running) {
            batch.clear()
            queue.drainTo(batch)
            if (batch.isNotEmpty() && running) {
                try {
                    sendBatched(batch)
                } catch (t: Throwable) {
                    if (!running) break
                    lastError = t.message ?: t.javaClass.simpleName
                }
            }
            try {
                Thread.sleep(BATCH_INTERVAL_MS)
            } catch (_: InterruptedException) {
                break
            }
        }
    }

    /** Divide las muestras en tantos datagramas como haga falta para respetar el MTU. */
    private fun sendBatched(samples: List<SensorSample>) {
        var start = 0
        while (start < samples.size) {
            var end = samples.size
            var payload: ByteArray
            while (true) {
                payload = serialize(samples.subList(start, end))
                if (payload.size <= MAX_DATAGRAM_PAYLOAD || end - start <= 1) break
                end = start + (end - start) / 2
            }
            sendDatagram(payload, end - start)
            start = end
        }
    }

    private fun serialize(samples: List<SensorSample>): ByteArray {
        val batch = UdpBatch(
            seq = sequence,
            device = deviceName,
            model = modelName,
            timestamp = System.currentTimeMillis(),
            samples = samples
        )
        return gson.toJson(batch).toByteArray(Charsets.UTF_8)
    }

    private fun sendDatagram(payload: ByteArray, sampleCount: Int) {
        // Snapshot coherente: los escritores (start/stop/updateConfig) están
        // serializados con @Synchronized, así que esta lectura es consistente.
        val address = targetAddress ?: return
        val port = targetPort
        val currentSocket = socket ?: return
        val packet = DatagramPacket(payload, payload.size, address, port)
        currentSocket.send(packet)
        sequence += 1
        packetsSent.incrementAndGet()
        samplesSent.addAndGet(sampleCount.toLong())
        lastError = null
    }
}

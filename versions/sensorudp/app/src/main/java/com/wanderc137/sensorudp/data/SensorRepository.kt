package com.wanderc137.sensorudp.data

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorManager
import com.wanderc137.sensorudp.data.model.SensorInfo
import dagger.hilt.android.qualifiers.ApplicationContext
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Acceso al SensorManager y enumeración dinámica de sensores.
 */
@Singleton
class SensorRepository @Inject constructor(
    @ApplicationContext context: Context
) {
    private val sensorManager: SensorManager =
        context.getSystemService(Context.SENSOR_SERVICE) as SensorManager

    fun sensorManager(): SensorManager = sensorManager

    /** Enumera TODOS los sensores del dispositivo, sin hardcodear ninguno. */
    fun listSensors(): List<SensorInfo> =
        sensorManager.getSensorList(Sensor.TYPE_ALL).map(SensorInfo::from)
}

package com.wanderc137.sensorudp.data.model

import android.hardware.Sensor

/**
 * Información inmutable de un sensor detectado en el dispositivo.
 * La enumeración es dinámica: nunca se hardcodea la lista de sensores.
 */
data class SensorInfo(
    val name: String,
    val type: Int,
    val typeName: String,
    val vendor: String,
    val version: Int,
    val resolution: Float,
    val maxRange: Float,
    val power: Float,
    val reportingMode: Int,
    val reportingModeName: String,
    val isWakeUp: Boolean
) {
    companion object {

        fun from(sensor: Sensor): SensorInfo = SensorInfo(
            name = sensor.name,
            type = sensor.type,
            typeName = typeName(sensor.type),
            vendor = sensor.vendor,
            version = sensor.version,
            resolution = sensor.resolution,
            maxRange = sensor.maximumRange,
            power = sensor.power,
            reportingMode = sensor.reportingMode,
            reportingModeName = reportingModeName(sensor.reportingMode),
            isWakeUp = sensor.isWakeUpSensor
        )

        /** Nombre legible del tipo de sensor (no enumera sensores, solo etiqueta el tipo). */
        fun typeName(type: Int): String = when (type) {
            Sensor.TYPE_ACCELEROMETER -> "ACCELEROMETER"
            Sensor.TYPE_MAGNETIC_FIELD -> "MAGNETIC_FIELD"
            Sensor.TYPE_ORIENTATION -> "ORIENTATION"
            Sensor.TYPE_GYROSCOPE -> "GYROSCOPE"
            Sensor.TYPE_LIGHT -> "LIGHT"
            Sensor.TYPE_PRESSURE -> "PRESSURE"
            Sensor.TYPE_TEMPERATURE -> "TEMPERATURE"
            Sensor.TYPE_PROXIMITY -> "PROXIMITY"
            Sensor.TYPE_GRAVITY -> "GRAVITY"
            Sensor.TYPE_LINEAR_ACCELERATION -> "LINEAR_ACCELERATION"
            Sensor.TYPE_ROTATION_VECTOR -> "ROTATION_VECTOR"
            Sensor.TYPE_RELATIVE_HUMIDITY -> "RELATIVE_HUMIDITY"
            Sensor.TYPE_AMBIENT_TEMPERATURE -> "AMBIENT_TEMPERATURE"
            Sensor.TYPE_MAGNETIC_FIELD_UNCALIBRATED -> "MAGNETIC_FIELD_UNCALIBRATED"
            Sensor.TYPE_GAME_ROTATION_VECTOR -> "GAME_ROTATION_VECTOR"
            Sensor.TYPE_GYROSCOPE_UNCALIBRATED -> "GYROSCOPE_UNCALIBRATED"
            Sensor.TYPE_SIGNIFICANT_MOTION -> "SIGNIFICANT_MOTION"
            Sensor.TYPE_STEP_DETECTOR -> "STEP_DETECTOR"
            Sensor.TYPE_STEP_COUNTER -> "STEP_COUNTER"
            Sensor.TYPE_GEOMAGNETIC_ROTATION_VECTOR -> "GEOMAGNETIC_ROTATION_VECTOR"
            Sensor.TYPE_HEART_RATE -> "HEART_RATE"
            Sensor.TYPE_POSE_6DOF -> "POSE_6DOF"
            Sensor.TYPE_STATIONARY_DETECT -> "STATIONARY_DETECT"
            Sensor.TYPE_MOTION_DETECT -> "MOTION_DETECT"
            Sensor.TYPE_HEART_BEAT -> "HEART_BEAT"
            Sensor.TYPE_LOW_LATENCY_OFFBODY_DETECT -> "LOW_LATENCY_OFFBODY_DETECT"
            Sensor.TYPE_ACCELEROMETER_UNCALIBRATED -> "ACCELEROMETER_UNCALIBRATED"
            Sensor.TYPE_HINGE_ANGLE -> "HINGE_ANGLE"
            else -> "TYPE_$type"
        }

        fun reportingModeName(mode: Int): String = when (mode) {
            Sensor.REPORTING_MODE_CONTINUOUS -> "CONTINUOUS"
            Sensor.REPORTING_MODE_ON_CHANGE -> "ON_CHANGE"
            Sensor.REPORTING_MODE_ONE_SHOT -> "ONE_SHOT"
            Sensor.REPORTING_MODE_SPECIAL_TRIGGER -> "SPECIAL_TRIGGER"
            else -> "MODE_$mode"
        }
    }
}

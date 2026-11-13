package com.wanderc137.sensorudp.data.model

/**
 * Una única lectura de sensor lista para serializar en el lote UDP.
 * `values` es una copia, porque SensorEvent reutiliza el array original.
 */
data class SensorSample(
    val sensor: String,
    val type: Int,
    val typeName: String,
    val eventTimestamp: Long,
    val values: List<Float>
)

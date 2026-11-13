package com.wanderc137.sensorudp.data.model

/**
 * Lote de muestras tal como viaja en un datagrama UDP (JSON UTF-8).
 * Esquema: {seq, device, model, timestamp, samples[]}
 */
data class UdpBatch(
    val seq: Long,
    val device: String,
    val model: String,
    val timestamp: Long,
    val samples: List<SensorSample>
)

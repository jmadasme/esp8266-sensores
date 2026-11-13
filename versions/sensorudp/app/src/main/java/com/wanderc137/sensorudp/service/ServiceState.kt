package com.wanderc137.sensorudp.service

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import javax.inject.Inject
import javax.inject.Singleton

/**
 * Estado observable del servicio de transmisión, compartido entre el servicio
 * (productor) y el ViewModel/UI (consumidores).
 */
data class ServiceState(
    val isRunning: Boolean = false,
    val targetIp: String = "",
    val targetPort: Int = 0,
    val packetsSent: Long = 0,
    val samplesSent: Long = 0,
    val droppedSamples: Long = 0,
    val activeSensors: Int = 0,
    val lastError: String? = null
)

@Singleton
class ServiceStateHolder @Inject constructor() {

    private val _state = MutableStateFlow(ServiceState())
    val state: StateFlow<ServiceState> = _state.asStateFlow()

    fun update(transform: (ServiceState) -> ServiceState) {
        _state.update(transform)
    }

    fun current(): ServiceState = _state.value
}

package com.wanderc137.sensorudp.ui

import android.app.Application
import androidx.core.content.ContextCompat
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.wanderc137.sensorudp.R
import com.wanderc137.sensorudp.data.SensorRepository
import com.wanderc137.sensorudp.data.UdpSettingsRepository
import com.wanderc137.sensorudp.data.model.SensorInfo
import com.wanderc137.sensorudp.network.UdpConfig
import com.wanderc137.sensorudp.service.SensorUdpService
import com.wanderc137.sensorudp.service.ServiceState
import com.wanderc137.sensorudp.service.ServiceStateHolder
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import javax.inject.Inject

@HiltViewModel
class MainViewModel @Inject constructor(
    application: Application,
    private val settingsRepository: UdpSettingsRepository,
    private val sensorRepository: SensorRepository,
    private val serviceStateHolder: ServiceStateHolder
) : AndroidViewModel(application) {

    data class UiState(
        val ip: String = "",
        val port: String = "",
        val inputError: String? = null,
        val service: ServiceState = ServiceState(),
        val sensors: List<SensorInfo> = emptyList()
    ) {
        val totalSensors: Int get() = sensors.size
    }

    private val ipInput = MutableStateFlow("")
    private val portInput = MutableStateFlow(UdpConfig.DEFAULT_PORT.toString())
    private val inputError = MutableStateFlow<String?>(null)

    private val detectedSensors: List<SensorInfo> = sensorRepository.listSensors()

    val uiState: StateFlow<UiState> = combine(
        ipInput,
        portInput,
        inputError,
        serviceStateHolder.state
    ) { ip, port, error, service ->
        UiState(
            ip = ip,
            port = port,
            inputError = error,
            service = service,
            sensors = detectedSensors
        )
    }.stateIn(
        scope = viewModelScope,
        started = SharingStarted.WhileSubscribed(5_000),
        initialValue = UiState(
            port = UdpConfig.DEFAULT_PORT.toString(),
            sensors = detectedSensors
        )
    )

    init {
        viewModelScope.launch {
            ipInput.value = settingsRepository.destinationIp.first()
            portInput.value = settingsRepository.destinationPort.first().toString()
        }
    }

    fun onIpChange(value: String) {
        ipInput.value = value.filterNot { it.isWhitespace() }
        inputError.value = null
    }

    fun onPortChange(value: String) {
        portInput.value = value.filter { it.isDigit() }.take(5)
        inputError.value = null
    }

    /** Valida y arranca la transmisión como foreground service. */
    fun start() {
        val config = validatedConfig() ?: return
        val app = getApplication<Application>()
        viewModelScope.launch { settingsRepository.save(config.ip, config.port) }
        serviceStateHolder.update { it.copy(lastError = null) }
        ContextCompat.startForegroundService(
            app,
            SensorUdpService.startIntent(app, config.ip, config.port)
        )
    }

    /** Aplica IP/puerto nuevos en caliente sin detener el servicio. */
    fun applyConfig() {
        val config = validatedConfig() ?: return
        val app = getApplication<Application>()
        viewModelScope.launch { settingsRepository.save(config.ip, config.port) }
        app.startService(SensorUdpService.updateIntent(app, config.ip, config.port))
    }

    fun stop() {
        val app = getApplication<Application>()
        app.startService(SensorUdpService.stopIntent(app))
    }

    private fun validatedConfig(): UdpConfig? {
        val context = getApplication<Application>()
        val ip = ipInput.value.trim()
        val port = portInput.value.toIntOrNull()
        val error = when {
            ip.isBlank() || !UdpConfig.isValidIp(ip) ->
                context.getString(R.string.error_invalid_ip)

            port == null || !UdpConfig.isValidPort(port) ->
                context.getString(R.string.error_invalid_port)

            else -> null
        }
        if (error != null || port == null) {
            inputError.value = error ?: context.getString(R.string.error_invalid_port)
            return null
        }
        return UdpConfig(ip, port)
    }
}

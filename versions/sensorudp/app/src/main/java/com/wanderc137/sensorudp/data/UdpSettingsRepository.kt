package com.wanderc137.sensorudp.data

import android.content.Context
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.intPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import com.wanderc137.sensorudp.network.UdpConfig
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map
import javax.inject.Inject
import javax.inject.Singleton

private val Context.settingsDataStore by preferencesDataStore(name = "sensorudp_settings")

/**
 * Persistencia de la configuración UDP (IP y puerto destino) con DataStore Preferences.
 */
@Singleton
class UdpSettingsRepository @Inject constructor(
    @ApplicationContext private val context: Context
) {
    private val keyIp = stringPreferencesKey("destination_ip")
    private val keyPort = intPreferencesKey("destination_port")

    val destinationIp: Flow<String> =
        context.settingsDataStore.data.map { it[keyIp] ?: "" }

    val destinationPort: Flow<Int> =
        context.settingsDataStore.data.map { it[keyPort] ?: UdpConfig.DEFAULT_PORT }

    suspend fun save(ip: String, port: Int) {
        context.settingsDataStore.edit { prefs ->
            prefs[keyIp] = ip
            prefs[keyPort] = port
        }
    }
}

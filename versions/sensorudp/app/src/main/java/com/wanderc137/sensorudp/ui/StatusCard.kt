package com.wanderc137.sensorudp.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.wanderc137.sensorudp.R
import com.wanderc137.sensorudp.service.ServiceState

@Composable
fun StatusCard(
    service: ServiceState,
    batteryExempt: Boolean,
    networkLabel: String,
    modifier: Modifier = Modifier
) {
    Card(modifier = modifier.fillMaxWidth()) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(8.dp)
        ) {
            Text(
                text = stringResource(R.string.status_title),
                style = MaterialTheme.typography.titleMedium
            )
            StatusRow(
                label = stringResource(R.string.label_status),
                value = stringResource(
                    if (service.isRunning) R.string.status_running else R.string.status_stopped
                ),
                highlight = service.isRunning
            )
            StatusRow(
                label = stringResource(R.string.label_destination),
                value = if (service.targetIp.isBlank()) {
                    "—"
                } else {
                    "${service.targetIp}:${service.targetPort}"
                }
            )
            StatusRow(stringResource(R.string.label_packets), service.packetsSent.toString())
            StatusRow(stringResource(R.string.label_samples), service.samplesSent.toString())
            StatusRow(stringResource(R.string.label_dropped), service.droppedSamples.toString())
            StatusRow(
                stringResource(R.string.label_active_sensors),
                service.activeSensors.toString()
            )
            StatusRow(
                stringResource(R.string.label_battery),
                stringResource(
                    if (batteryExempt) R.string.battery_exempt else R.string.battery_optimized
                )
            )
            StatusRow(stringResource(R.string.label_network), networkLabel)
            service.lastError?.let { error ->
                Text(
                    text = stringResource(R.string.label_last_error) + ": " + error,
                    color = MaterialTheme.colorScheme.error,
                    style = MaterialTheme.typography.bodySmall
                )
            }
        }
    }
}

@Composable
private fun StatusRow(label: String, value: String, highlight: Boolean = false) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodyMedium,
            fontWeight = if (highlight) FontWeight.Bold else FontWeight.Normal
        )
    }
}

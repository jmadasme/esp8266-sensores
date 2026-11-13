package com.wanderc137.sensorudp.ui

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.net.Uri
import android.os.Build
import android.os.PowerManager
import android.provider.Settings
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CenterAlignedTopAppBar
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import androidx.hilt.navigation.compose.hiltViewModel
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.wanderc137.sensorudp.R

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(viewModel: MainViewModel = hiltViewModel()) {
    val uiState by viewModel.uiState.collectAsStateWithLifecycle()
    val context = LocalContext.current

    val notificationPlatformSupported = Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU
    var notificationGranted by remember { mutableStateOf(hasNotificationPermission(context)) }
    var showPermissionDialog by remember { mutableStateOf(false) }

    // I-002: si el usuario intentó iniciar y concede el permiso, se arranca
    // recién entonces (se espera la respuesta del permiso).
    var startWhenGranted by remember { mutableStateOf(false) }

    val notificationPermissionLauncher = rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted ->
        notificationGranted = granted
        if (granted && startWhenGranted) {
            viewModel.start()
        }
        startWhenGranted = false
    }

    // I-006: re-chequear el permiso al volver la app al frente (p. ej. si el
    // usuario lo concede desde Ajustes del sistema).
    val lifecycleOwner = LocalLifecycleOwner.current
    DisposableEffect(lifecycleOwner) {
        val observer = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_RESUME) {
                notificationGranted = hasNotificationPermission(context)
            }
        }
        lifecycleOwner.lifecycle.addObserver(observer)
        onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
    }

    // Se recalculan en cada recomposición (los contadores refrescan 1 vez por segundo).
    val batteryExempt = isIgnoringBatteryOptimizations(context)
    val networkLabel = currentNetworkLabel(context)
    val notificationPermissionMissing = notificationPlatformSupported && !notificationGranted

    Scaffold(
        topBar = {
            CenterAlignedTopAppBar(title = { Text(stringResource(R.string.app_name)) })
        }
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(innerPadding)
                .verticalScroll(rememberScrollState())
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            ConfigCard(
                uiState = uiState,
                onIpChange = viewModel::onIpChange,
                onPortChange = viewModel::onPortChange,
                onStartStop = {
                    if (uiState.service.isRunning) {
                        viewModel.stop()
                    } else if (notificationPermissionMissing) {
                        // I-002: mostrar diálogo informativo y no arrancar
                        // hasta que el usuario responda al permiso.
                        startWhenGranted = true
                        showPermissionDialog = true
                    } else {
                        viewModel.start()
                    }
                },
                onApply = viewModel::applyConfig
            )

            if (notificationPermissionMissing) {
                // I-006: acción dedicada para solicitar POST_NOTIFICATIONS.
                OutlinedButton(
                    onClick = {
                        notificationPermissionLauncher.launch(
                            Manifest.permission.POST_NOTIFICATIONS
                        )
                    },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text(stringResource(R.string.action_request_notifications))
                }
            }

            if (!batteryExempt) {
                OutlinedButton(
                    onClick = { openBatterySettings(context) },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text(stringResource(R.string.action_request_battery))
                }
            }

            StatusCard(
                service = uiState.service,
                batteryExempt = batteryExempt,
                networkLabel = networkLabel
            )

            SensorList(
                sensors = uiState.sensors,
                modifier = Modifier.fillMaxWidth()
            )
        }
    }

    if (showPermissionDialog) {
        AlertDialog(
            onDismissRequest = {
                showPermissionDialog = false
                startWhenGranted = false
            },
            title = { Text(stringResource(R.string.notification_permission_title)) },
            text = { Text(stringResource(R.string.notification_permission_message)) },
            confirmButton = {
                TextButton(
                    onClick = {
                        showPermissionDialog = false
                        notificationPermissionLauncher.launch(
                            Manifest.permission.POST_NOTIFICATIONS
                        )
                    }
                ) {
                    Text(stringResource(R.string.notification_permission_grant))
                }
            },
            dismissButton = {
                TextButton(
                    onClick = {
                        showPermissionDialog = false
                        startWhenGranted = false
                    }
                ) {
                    Text(stringResource(R.string.action_cancel))
                }
            }
        )
    }
}

@Composable
private fun ConfigCard(
    uiState: MainViewModel.UiState,
    onIpChange: (String) -> Unit,
    onPortChange: (String) -> Unit,
    onStartStop: () -> Unit,
    onApply: () -> Unit
) {
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            Text(
                text = stringResource(R.string.section_config),
                style = MaterialTheme.typography.titleMedium
            )
            OutlinedTextField(
                value = uiState.ip,
                onValueChange = onIpChange,
                label = { Text(stringResource(R.string.ip_label)) },
                placeholder = { Text(stringResource(R.string.ip_hint)) },
                singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Uri),
                isError = uiState.inputError != null,
                modifier = Modifier.fillMaxWidth()
            )
            OutlinedTextField(
                value = uiState.port,
                onValueChange = onPortChange,
                label = { Text(stringResource(R.string.port_label)) },
                placeholder = { Text(stringResource(R.string.port_hint)) },
                singleLine = true,
                keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                isError = uiState.inputError != null,
                modifier = Modifier.fillMaxWidth()
            )
            uiState.inputError?.let { message ->
                Text(
                    text = message,
                    color = MaterialTheme.colorScheme.error,
                    style = MaterialTheme.typography.bodySmall
                )
            }
            Button(
                onClick = onStartStop,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text(
                    stringResource(
                        if (uiState.service.isRunning) {
                            R.string.action_stop
                        } else {
                            R.string.action_start
                        }
                    )
                )
            }
            if (uiState.service.isRunning) {
                OutlinedButton(
                    onClick = onApply,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text(stringResource(R.string.action_apply))
                }
            }
        }
    }
}

private fun hasNotificationPermission(context: Context): Boolean {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) return true
    return ContextCompat.checkSelfPermission(
        context,
        Manifest.permission.POST_NOTIFICATIONS
    ) == PackageManager.PERMISSION_GRANTED
}

private fun isIgnoringBatteryOptimizations(context: Context): Boolean {
    val powerManager = context.getSystemService(Context.POWER_SERVICE) as PowerManager
    return powerManager.isIgnoringBatteryOptimizations(context.packageName)
}

private fun openBatterySettings(context: Context) {
    try {
        context.startActivity(
            Intent(
                Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS,
                Uri.parse("package:${context.packageName}")
            )
        )
    } catch (_: Exception) {
        try {
            context.startActivity(Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS))
        } catch (_: Exception) {
            // Sin actividad disponible; el usuario puede hacerlo manualmente en Ajustes.
        }
    }
}

private fun currentNetworkLabel(context: Context): String {
    val connectivityManager =
        context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
    val capabilities =
        connectivityManager.getNetworkCapabilities(connectivityManager.activeNetwork)
            ?: return context.getString(R.string.network_none)
    return when {
        capabilities.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) ->
            context.getString(R.string.network_wifi)

        capabilities.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) ->
            context.getString(R.string.network_cellular)

        capabilities.hasTransport(NetworkCapabilities.TRANSPORT_ETHERNET) ->
            context.getString(R.string.network_ethernet)

        else -> context.getString(R.string.network_none)
    }
}

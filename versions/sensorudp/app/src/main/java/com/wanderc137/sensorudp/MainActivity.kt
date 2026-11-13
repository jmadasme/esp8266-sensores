package com.wanderc137.sensorudp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import com.wanderc137.sensorudp.ui.MainScreen
import com.wanderc137.sensorudp.ui.theme.SensorUdpTheme
import dagger.hilt.android.AndroidEntryPoint

@AndroidEntryPoint
class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            SensorUdpTheme {
                MainScreen()
            }
        }
    }
}

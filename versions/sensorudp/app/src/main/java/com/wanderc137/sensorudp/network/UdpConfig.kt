package com.wanderc137.sensorudp.network

/**
 * Configuración de destino UDP (IP + puerto).
 */
data class UdpConfig(
    val ip: String,
    val port: Int
) {
    companion object {
        const val DEFAULT_PORT = 5005

        private val IPV4_REGEX = Regex(
            "^((25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)\\.){3}(25[0-5]|2[0-4]\\d|1\\d\\d|[1-9]?\\d)$"
        )

        private val HOSTNAME_REGEX = Regex(
            "^[A-Za-z0-9]([A-Za-z0-9.\\-]{0,253}[A-Za-z0-9])?$"
        )

        /** Acepta IPv4 numérica o un nombre de host válido. */
        fun isValidIp(ip: String): Boolean {
            val trimmed = ip.trim()
            if (trimmed.isEmpty()) return false
            return IPV4_REGEX.matches(trimmed) || HOSTNAME_REGEX.matches(trimmed)
        }

        fun isValidPort(port: Int): Boolean = port in 1..65535
    }
}

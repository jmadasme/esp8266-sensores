# Reglas ProGuard — SensorUDP
# Gson usa reflexión sobre los modelos; conservar sus campos.
-keep class com.wanderc137.sensorudp.data.model.** { *; }
-keepattributes Signature
-keepattributes *Annotation*

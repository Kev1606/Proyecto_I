# Cargar la biblioteca necesaria
library(ggplot2)

# Cargar los datos desde el archivo CSV
datos <- read.csv("Analisis/logfile.csv")

# Mostrar una vista previa de los datos
head(datos)

# Calcular el tiempo promedio por subproceso
promedio_tiempo <- aggregate(Tiempo ~ Subproceso, data = datos, FUN = mean)

# Ordenar los subprocesos por tiempo promedio
promedio_tiempo <- promedio_tiempo[order(promedio_tiempo$Tiempo),]

# Graficar los subprocesos más rápidos
ggplot(promedio_tiempo, aes(x = factor(Subproceso), y = Tiempo)) +
  geom_bar(stat = "identity") +
  labs(title = "Subprocesos más rápidos",
       x = "Subproceso",
       y = "Tiempo promedio") +
  theme(axis.text.x = element_text(angle = 90, hjust = 1))

# Contar cuántos archivos se ejecutaron
total_archivos <- nrow(unique(datos[, c("Archivo", "Subproceso")]))
cat("Total de archivos ejecutados:", total_archivos)

# Guía de Migración: Proyecto de Sable de Luz (saber-poc)

Esta guía detalla cómo crear un proyecto desde cero, sincronizarlo con los repositorios base y migrar la lógica del proyecto antiguo (`blink`) de forma limpia y segura.

## 1. Configuración del Nuevo Entorno (Terreno Virgen)

Ejecuta esto en una carpeta **completamente nueva**:

```bash
# 1. Crear el nuevo proyecto
mkdir saber-poc
cd saber-poc
git init

# 2. Configurar remotos
git remote add template https://github.com/tmarcokr/esp-idf-template.git
git remote add componentes https://github.com/tmarcokr/esp-idf-components.git
```

---

## 2. Sincronización Selectiva (Cimentación)

### 2.1 Traer la Template (Estructura y Agentes)
```bash
git fetch template
git merge template/main --no-commit --no-ff --allow-unrelated-histories

# Limpieza quirúrgica: solo nos quedamos con la inteligencia y configuración
git reset HEAD
git add .agents/ .vscode/ GEMINI.md .gitignore
git restore .
git clean -fd
git commit -m "chore: initial sync from esp-idf-template"
```

### 2.2 Traer Componentes (Librerías de Bajo Nivel)
```bash
git fetch componentes
git merge componentes/main --no-commit --no-ff --allow-unrelated-histories

# Limpieza quirúrgica: solo traemos las librerías necesarias
git reset HEAD
git add components/
git restore .
git clean -fd
git commit -m "chore: sync components from esp-idf-components"
```

---

## 3. Extracción Quirúrgica de Lógica (Desde el Proyecto Antiguo)

Define la ruta de tu proyecto antiguo (ej: `../blink`) y ejecuta la copia de los elementos críticos:

```bash
# Define aquí la ruta a tu proyecto antiguo
export OLD_PROJECT_PATH="../blink"

# 1. Crear estructura en main
mkdir -p main/samples/rgb
mkdir -p main/samples/smoothswing

# 2. Copiar Interfaz y Samples específicos
cp $OLD_PROJECT_PATH/main/samples/ISample.hpp main/samples/
cp $OLD_PROJECT_PATH/main/samples/rgb/LedSample.* main/samples/rgb/
cp $OLD_PROJECT_PATH/main/samples/smoothswing/SmoothSwingSample.* main/samples/smoothswing/

# 3. Copiar el Orquestador
cp $OLD_PROJECT_PATH/main/main.cpp main/
```

---

## 4. Ajustes Finales de Compilación

### 4.1 Configurar `main/CMakeLists.txt`
Asegúrate de que el archivo `main/CMakeLists.txt` en el nuevo proyecto se vea así:

```cmake
idf_component_register(
    SRCS "main.cpp"
         "samples/rgb/LedSample.cpp"
         "samples/smoothswing/SmoothSwingSample.cpp"
    INCLUDE_DIRS "." 
                 "samples" 
                 "samples/rgb" 
                 "samples/smoothswing"
    REQUIRES rgb_led sd_card audio_engine mpu6050 gpio_button
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
```

---

## 5. Limpieza del Orquestador (`main.cpp`)

En el nuevo `main.cpp`, borra o comenta las referencias a samples que ya no están presentes (SdSample, AudioSample, etc.). Mantén solo:
- `led_sample` para feedback visual.
- `ss_sample` para la física del sable.

---

## 6. Verificación y Construcción

```bash
idf.py set-target esp32c6
idf.py build
```

---

## 7. Ventajas de este Flujo (Análisis Pro)

1. **Integridad del Original**: El proyecto `blink` queda intacto como backup.
2. **Cero Residuos**: Al empezar con una carpeta vacía y usar `git clean -fd`, garantizas que no hay archivos huérfanos del proyecto anterior.
3. **Control de Versiones Limpio**: El historial de Git de `saber-poc` empezará con los commits de sincronización, siendo mucho más fácil de seguir.
4. **Agentes Actualizados**: Al sincronizar con la template al inicio, te aseguras de tener las últimas instrucciones para los Agentes de IA.

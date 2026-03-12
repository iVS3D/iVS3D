<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de_DE">
<context>
    <name>MCFG::ModelSettingsWidget</name>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="75"/>
        <source>Model:</source>
        <translation>Modell:</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="78"/>
        <source>Select which neural network to use</source>
        <translation>Wählen Sie das Neuronale Netz aus</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="99"/>
        <source>Select Classes:</source>
        <translation>Klassenauswahl:</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="102"/>
        <source>Search...</source>
        <translation>Suchen...</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="106"/>
        <source>Invert</source>
        <translation>Auswahl umkehren</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="143"/>
        <source>▶  Model Configuration</source>
        <translation>▶  Modell Konfiguration</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="156"/>
        <source>▼</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="156"/>
        <source>▶</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="157"/>
        <source>  Model Configuration</source>
        <translation>  Modell Konfiguration</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="173"/>
        <source>Apply Mean/Std Normalization</source>
        <translation>Normalisierung duch Mittelwert/Standardabweichung</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="174"/>
        <source>Apply mean and standard deviation normalization</source>
        <translation>Mittelwert von Eingabe subtrahieren und durch Standardabweichung teilen</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="186"/>
        <source>Normalize input [0,255] → [0,1]</source>
        <translation>Eingabe normalisieren [0,255] → [0,1]</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="187"/>
        <source>Divide pixel values by 255 before applying normalization</source>
        <translation>Pixelwerte durch 255 teilen, bevor weitere Normalisierungen angewendet werden</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="195"/>
        <source>Input Alignment:</source>
        <translation>Eingabeausrichtung:</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="214"/>
        <source>ⓘ Configuration changes made here are temporary and will not be persisted. To save these settings permanently, manually update the model configuration JSON file.</source>
        <translation>ⓘ Änderungen an der Konfiguration sind temporär. Um diese permantent zu speichern, aktualisieren Sie die Modellkonfiguration in der zugehörigen JSON-Datei.</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="312"/>
        <source>No models found</source>
        <translation>Keine Modelle gefunden</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="314"/>
        <source>No detection models available</source>
        <translation>Keine Modelle verfügbar</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="318"/>
        <source>No detection models found in the model directory.</source>
        <translation>Keine Modelle im models-Ordner gefunden.</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="437"/>
        <location filename="../src/ModelSettingsWidget.cpp" line="572"/>
        <source>Ready</source>
        <translation>Bereit</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="497"/>
        <source>No classes defined in this model</source>
        <translation>Keine Klassen für dieses Modell hinterlegt</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="550"/>
        <source>💡 Add a .json configuration file with the same name as the model in the models directory.</source>
        <translation>💡 Erstellen Sie eine .json Datei mit dem selben Namen wie das Modell im models-Ordner.</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="553"/>
        <source>💡 Ensure the .onnx model file exists. Check the &apos;modelPath&apos; in the config or place a .onnx file with the same name as the config.</source>
        <translation>💡 Stellen Sie sicher, dass die .onnx Modell-Datei existiert. Überprüfen Sie den &apos;modelPath&apos; in der Kondifurationsdatei oder fügen Sie eine .onnx Datei mit dem selben Namen wie die Konfigurationsdatei in dem models-Ordner.</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="557"/>
        <source>💡 Check for duplicate class IDs or mismatched normalization vector sizes (mean/std).</source>
        <translation>💡 Überprüfen die die Konfigurationsdatei auf mehrfach verwendeter Klassen IDs oder unstimmige Vektorgrößen für mean oderstd.</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="560"/>
        <source>💡 This model may require a different version or configuration. Check the error details.</source>
        <translation>💡 Dieses Modell benmötigt evtl eine andere Version oder Konfiguration. Überprüfen Sie die Fehlermeldung.</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="563"/>
        <source>💡 Make sure to add your models and .json configurations to the models directory.</source>
        <translation>💡 Fügen sie Modelle und Konfigurationsdateinen (.json) in den models-Ordner hinzu.</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="574"/>
        <source>Missing Config</source>
        <translation>Fehlende Konfiguration</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="576"/>
        <source>Missing Model</source>
        <translation>Fehlendes Modell</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="578"/>
        <source>Invalid Config</source>
        <translation>Ungültige Konfiguration</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="580"/>
        <source>Incompatible</source>
        <translation>Inkompatibel</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="582"/>
        <source>Unknown</source>
        <translation>Unbekannt</translation>
    </message>
    <message>
        <location filename="../src/ModelSettingsWidget.cpp" line="794"/>
        <source>▶ Model Configuration</source>
        <translation>▶ Modellkonfiguration</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../src/ModelConfig.cpp" line="97"/>
        <source>Duplicate class ID found: %1</source>
        <translation>Mehrfach vorkommende Klassen ID gefunden: %1</translation>
    </message>
    <message>
        <location filename="../src/ModelConfig.cpp" line="105"/>
        <source>Unknown &lt;%1&gt;</source>
        <translation>Unbekannte Klasse &lt;%1&gt;</translation>
    </message>
</context>
</TS>

<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de_DE">
<context>
    <name>SegmentationPlugin</name>
    <message>
        <location filename="../segplugin.cpp" line="26"/>
        <source>No segmentation model is currently active. Please select a model in the plugin settings.</source>
        <translation>Es ist kein Modell zur Segmentierung ausgewählt. Bitte wählen Sie ein Modell in den Plugin Einstellungen aus.</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="43"/>
        <source>Failed to load neural network model: %1</source>
        <translation>Laden des Neuronalen Netzes gescheitert: %1</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="77"/>
        <source>No detection models found in the models directory.</source>
        <translation>Keine passenden Modelle gefunden.</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="137"/>
        <source>selectedModel is required in settings.</source>
        <translation>selectedModel wird in den Einstellungen benötigt.</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="144"/>
        <source>Failed to parse selected model from settings.</source>
        <translation>Laden des ausgewählten Modells aus den Einstellungen fehlgeschlagen.</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="167"/>
        <location filename="../segplugin.cpp" line="221"/>
        <source>An error occurred during model inference:
%1</source>
        <translation>Bei der Inferenz des Modells ist ein Fehler aufgetreten:
%1</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="177"/>
        <location filename="../segplugin.cpp" line="261"/>
        <source>An error occurred during mask computation.
%1</source>
        <translation>Beim berechnen der Maske ist ein Fehler aufgetreten:
%1</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="215"/>
        <source>Ran out of memory during inference!
Lower the working resolution to reduce memory usage.</source>
        <translation>Nicht genügend GPU Speicher für die Inferenz!
Reduzieren Sie die Arbeitsauflösung um den Speicherbedarf zu senken.</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="240"/>
        <source>An error occurred during colorization.
%1</source>
        <translation>Beim Einfärben ist ein Fehler aufgetreten:
%1</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="279"/>
        <source>Segmentation Preview (inference time: %1 ms)</source>
        <translation>Segmentierungsvorschau (Inferenzdauer: %1ms)</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="282"/>
        <source>Segmentation Preview (cached)</source>
        <translation>Segmentierungsvorschau (gespeichert)</translation>
    </message>
    <message>
        <location filename="../segplugin.cpp" line="296"/>
        <source>Segmentation Mask</source>
        <translation>Segmentierungsmaske</translation>
    </message>
    <message>
        <location filename="../segplugin.h" line="31"/>
        <source>Segmentation Masks</source>
        <translation>Segmentierungsmasken</translation>
    </message>
</context>
<context>
    <name>SegmentationSettingsWidget</name>
    <message>
        <location filename="../segsettingswidget.cpp" line="24"/>
        <source>Overlay Opacity:</source>
        <translation>Transparenz der Vorschau:</translation>
    </message>
    <message>
        <location filename="../segsettingswidget.cpp" line="28"/>
        <source>Adjust transparency of the segmentation overlay (0% = transparent, 100% = opaque)</source>
        <translation>Passt die Transparenz der Segmentierungsvorschau an (0% = transparent, 100% = undurchsichtig)</translation>
    </message>
</context>
</TS>

<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de_DE">
<context>
    <name>QObject</name>
    <message>
        <location filename="../optflowcontroller.h" line="42"/>
        <source>Stationary camera removal</source>
        <translation>Entferne stationäre Kameras</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.h" line="44"/>
        <source>Stationary threshold</source>
        <translation>Schwellwert für stationäre Kameras</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.h" line="45"/>
        <source>Removes all frames where the camera is stationary. A frame is declared stationary if its camera movement is lower than given percentage of the median of all camera movements.</source>
        <translation>Entfernt Bilder für welche die Kamera statisch ist. Eine Kamera gilt als statisch, wenn die Bewegung zwischen zwei Bildern niedriger ist als der gegebenen Anteil des gesamten Kamerabewegungsmedians.</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.h" line="46"/>
        <source>Sampling resolution</source>
        <translation>Herunterskalierte Auflösung</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.h" line="47"/>
        <source>Activate down sampling</source>
        <translation>Herunterskalieren aktivieren</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.h" line="48"/>
        <source>If enabled a resolution of 720p will be used for the algorithm to speed up computation. This however will hurt the accuracy of the result slightly. It however won&apos;t change the export resolution. This parameter will be disabled if the input resolution is lower or equal than 720p.</source>
        <translation>Wenn aktiviert werden Bilder während der Ausführung des Algorithmus auf 720p herunterskaliert, um eine schnellere Ausführung und mit geringerer Genauigkeit zu erzielen. Diese Einstellung beeinflusst nicht die Bildauflösung des Exportvorgangs.</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.h" line="49"/>
        <source>Reset Buffer</source>
        <translation>Puffer zurücksetzten</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.h" line="50"/>
        <source>Clears all already stored flow values. There are </source>
        <translation>Setzt alle bereits gepufferten Werte zurück. Aktuell sind </translation>
    </message>
    <message>
        <location filename="../optflowcontroller.h" line="51"/>
        <source> flow values currently buffered.</source>
        <translation> optische Flusswerte gespeichert.</translation>
    </message>
</context>
<context>
    <name>SmoothController</name>
    <message>
        <location filename="../optflowcontroller.cpp" line="30"/>
        <source>Excluding buffered values from computation list</source>
        <translation>Entferne gepufferte Werte von der Liste zu berchnender Paare</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.cpp" line="44"/>
        <source>Creating calculation units</source>
        <translation>Berechnungseinheiten werden erstellt</translation>
    </message>
    <message>
        <location filename="../optflowcontroller.cpp" line="108"/>
        <source>Calculating flow between frame </source>
        <translation>Berechne optischen Fluss zwischen den Bildern </translation>
    </message>
    <message>
        <location filename="../optflowcontroller.cpp" line="108"/>
        <source> and </source>
        <translation> und </translation>
    </message>
    <message>
        <location filename="../optflowcontroller.cpp" line="131"/>
        <source>Buffering values</source>
        <translation>Puffert Werte</translation>
    </message>
</context>
</TS>

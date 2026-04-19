import QtQuick 2.15
import QtQuick.Controls 2.15

AbstractButton {
    id: root

    property int numTeeth: 7
    property real toothWidthRatio: 0.8

    implicitWidth: 60
    implicitHeight: 60

    RotationAnimation on rotation {
        id: spinAnimation
        from: 0
        to: 360
        duration: 3000
        loops: Animation.Infinite
        running: root.hovered
    }

    background: Canvas {
        id: gearCanvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            var w = width;
            var h = height;
            ctx.clearRect(0, 0, w, h);

            var centerX = w / 2;
            var centerY = h / 2;
            var outerR = w * 0.45;   
            var innerR = w * 0.30; 
            var angleStep = 2 * Math.PI / root.numTeeth;
            var toothWidth = angleStep * root.toothWidthRatio;

            ctx.beginPath();
            for (var i = 0; i < root.numTeeth; i++) {
                var baseAngle = i * angleStep;
                var x1 = centerX + outerR * Math.cos(baseAngle - toothWidth/2);
                var y1 = centerY + outerR * Math.sin(baseAngle - toothWidth/2);
                var x2 = centerX + innerR * Math.cos(baseAngle);
                var y2 = centerY + innerR * Math.sin(baseAngle);
                var x3 = centerX + outerR * Math.cos(baseAngle + toothWidth/2);
                var y3 = centerY + outerR * Math.sin(baseAngle + toothWidth/2);

                if (i === 0) ctx.moveTo(x1, y1);
                else ctx.lineTo(x1, y1);
                ctx.lineTo(x2, y2);
                ctx.lineTo(x3, y3);
            }
            ctx.closePath();

            if (root.pressed) ctx.fillStyle = "#7aa5c0";
            else if (root.hovered) ctx.fillStyle = "#a0c0e0";
            else if (root.checked) ctx.fillStyle = "#8cb9d1";
            else ctx.fillStyle = "#c0d0e0";

            ctx.fill();
            ctx.strokeStyle = "#404040";

            ctx.lineWidth = root.hovered ? 2.5 : 1.5;
            ctx.stroke();

            ctx.beginPath();
            ctx.arc(centerX, centerY, w * 0.12, 0, 2 * Math.PI);
            ctx.fillStyle = '#a9c1c4';
            ctx.fill();
            ctx.strokeStyle = "#404040";
            ctx.lineWidth = root.hovered ? 2.0 : 1.0; 
            ctx.stroke();
        }

        Connections {
            target: root
            function onPressedChanged() { gearCanvas.requestPaint(); }
            function onHoveredChanged() { gearCanvas.requestPaint(); }
            function onCheckedChanged() { gearCanvas.requestPaint(); }
        }
    }

    contentItem: Item {}
}
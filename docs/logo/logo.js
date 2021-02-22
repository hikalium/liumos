const iconSize = 1024;
var draw = SVG().addTo('#logo').size(iconSize, iconSize);

const drawArc =
    (draw, r, w, cx, cy, rad0, rad1, isBiggerArc, isClockwise, color) => {
      const p0x = cx + Math.cos(rad0) * r;
      const p0y = cy + Math.sin(rad0) * r;
      const p1x = cx + Math.cos(rad1) * r;
      const p1y = cy + Math.sin(rad1) * r;
      draw.path(`M ${p0x} ${p0y} A ${r} ${r} 0 ${isBiggerArc ? 1 : 0} ${
                    isClockwise ? 1 : 0} ${p1x} ${p1y}`)
          .css({
            stroke: color,
            fill: 'transparent',
            'stroke-width': w,
            'stroke-linecap': 'round',
            'stroke-linejoin': 'bevel'
          });
    }

const arcList = [
  {
    // L
    r: 0.8,
    rad0: Math.PI * 0,
    rad1: Math.PI * 1.5,
    isBiggerArc: true,
    isClockwise: true,
    color: '#84EC66',
  },
  {
    // I
    r: 0.6,
    rad0: -Math.PI * 1 / 2,
    rad1: Math.PI * 1 / 2,
    isBiggerArc: true,
    isClockwise: true,
    color: '#f2BA69',
  },
  {
    // U
    r: 0.4,
    rad0: -Math.PI * 1 / 4,
    rad1: -Math.PI * 3 / 4,
    isBiggerArc: true,
    isClockwise: true,
    color: '#D1422E',
  },
  {
    // M
    r: 0.2,
    rad0: Math.PI * 1 / 4,
    rad1: Math.PI * 3 / 4,
    isBiggerArc: false,
    isClockwise: true,
    color: '#A022AB',
  },
];
for (a of arcList) {
  drawArc(
      draw, a.r * iconSize / 2, 32, iconSize / 2, iconSize / 2, a.rad0, a.rad1,
      a.isBiggerArc, a.isClockwise, a.color);
}

const download = () => {
  var svg = document.querySelector('svg');
  var svgData = new XMLSerializer().serializeToString(svg);
  var canvas = document.createElement('canvas');
  canvas.width = svg.width.baseVal.value;
  canvas.height = svg.height.baseVal.value;

  var ctx = canvas.getContext('2d');
  var image = new Image;
  image.onload = function() {
    ctx.drawImage(image, 0, 0);
    var a = document.createElement('a');
    a.href = canvas.toDataURL('image/png');
    a.setAttribute('download', 'image.png');
    a.dispatchEvent(new MouseEvent('click'));
  };
  image.src = 'data:image/svg+xml;charset=utf-8;base64,' +
      btoa(unescape(encodeURIComponent(svgData)));
}

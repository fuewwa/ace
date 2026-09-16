(function () {
  if (window.matchMedia("(pointer: coarse)").matches) return;

  if (document.body) {
    init();
  } else {
    document.addEventListener("DOMContentLoaded", init);
  }

  function init() {

  var glow = document.createElement("div");
  glow.id = "cursor-glow";
  document.body.appendChild(glow);

  var style = document.createElement("style");
  style.textContent =
    "#cursor-glow{" +
    "position:fixed;" +
    "top:0;left:0;" +
    "width:600px;height:600px;" +
    "pointer-events:none;" +
    "z-index:9999;" +
    "border-radius:50%;" +
    "background:radial-gradient(circle, rgba(82,192,255,0.16) 0%, rgba(82,192,255,0.06) 35%, rgba(82,192,255,0) 70%);" +
    "transform:translate(-50%,-50%);" +
    "opacity:0;" +
    "transition:opacity .3s ease;" +
    "will-change:transform;" +
    "}";
  document.head.appendChild(style);

  var targetX = window.innerWidth / 2;
  var targetY = window.innerHeight / 2;
  var currentX = targetX;
  var currentY = targetY;
  var active = false;

  function onMove(e) {
    targetX = e.clientX;
    targetY = e.clientY;
    if (!active) {
      active = true;
      glow.style.opacity = "1";
    }
  }

  function onLeave() {
    active = false;
    glow.style.opacity = "0";
  }

  function tick() {
    currentX += (targetX - currentX) * 0.12;
    currentY += (targetY - currentY) * 0.12;
    glow.style.transform =
      "translate(" + (currentX - 300) + "px, " + (currentY - 300) + "px)";
    requestAnimationFrame(tick);
  }

  window.addEventListener("mousemove", onMove);
  document.addEventListener("mouseleave", onLeave);
  requestAnimationFrame(tick);

  }
})();

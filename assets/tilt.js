(function () {
  if (window.matchMedia("(pointer: coarse)").matches) return;

  if (document.body) {
    init();
  } else {
    document.addEventListener("DOMContentLoaded", init);
  }

  function init() {
    var target = document.querySelector(".screen");
    if (!target) return;

    target.style.display = "inline-block";
    target.style.transformStyle = "preserve-3d";
    target.style.willChange = "transform";
    target.style.transition = "transform .15s ease-out";

    var maxTilt = 10;

    function onMove(e) {
      var rect = target.getBoundingClientRect();
      var x = (e.clientX - rect.left) / rect.width;
      var y = (e.clientY - rect.top) / rect.height;

      var rotateY = (x - 0.5) * maxTilt * 2;
      var rotateX = (0.5 - y) * maxTilt * 2;

      target.style.transform =
        "perspective(900px) rotateX(" + rotateX + "deg) rotateY(" +
        rotateY + "deg) scale3d(1.02, 1.02, 1.02)";
    }

    function onLeave() {
      target.style.transform =
        "perspective(900px) rotateX(0deg) rotateY(0deg) scale3d(1, 1, 1)";
    }

    target.addEventListener("mousemove", onMove);
    target.addEventListener("mouseleave", onLeave);
  }
})();

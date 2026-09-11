(function () {
  "use strict";
  var $ = function (id) { return document.getElementById(id); };
  var api = null, busy = false;

  var SAMPLES = [
    { id: "s1", name: "source.pla", note: "12/8", key: "sample-1" },
    { id: "s2", name: "alu4 (small)", note: "10/6", key: "sample-2" },
    { id: "s3", name: "alu4 (large)", note: "14/8", key: "sample-3" }
  ];

  function sampleText(key) {
    var el = document.getElementById(key);
    return el ? el.textContent.replace(/^\n/, "") : "";
  }

  function setStatus(text, kind) {
    var s = $("status"); s.className = kind || "";
    $("status-text").textContent = text;
  }

  function countIO(text) {
    function n(dir) {
      var m = text.match(new RegExp("^\\." + dir + "\\s+(.*)$", "m"));
      return m ? m[1].trim().split(/\s+/).filter(Boolean).length : 0;
    }
    var names = (text.match(/^\.names\b/gm) || []).length;
    return { pi: n("inputs"), po: n("outputs"), names: names };
  }

  function refreshInMeta() {
    var c = countIO($("src").value);
    $("in-meta").textContent = c.pi + " PI · " + c.po + " PO · " + c.names + " .names";
  }

  function nextFrame() {
    return new Promise(function (r) {
      requestAnimationFrame(function () { requestAnimationFrame(function () { setTimeout(r, 0); }); });
    });
  }

  function mapOnce(text, k) {
    var out = api.map(text, k);
    var err = api.err();
    if (err) return { error: err };
    return { blif: out, luts: api.luts(), depth: api.depth() };
  }

  async function run() {
    if (!api || busy) return;
    var text = $("src").value;
    var k = parseInt($("k").value, 10);
    if (!text.trim()) { setStatus("Nothing to map — paste a netlist or pick a circuit.", "err"); return; }

    busy = true;
    $("run").disabled = true; $("sweep").disabled = true;
    setStatus("Mapping at k = " + k + "…");
    await nextFrame();

    var t0 = performance.now();
    var r = mapOnce(text, k);
    var ms = Math.round(performance.now() - t0);

    if (r.error) {
      setStatus(r.error, "err");
      $("out").textContent = "Run the mapper to see the LUT network.";
      $("out").className = "code empty";
      $("copy").disabled = true;
      ["s-luts", "s-depth", "s-pi", "s-po", "s-k"].forEach(function (id) { $(id).textContent = "—"; });
    } else {
      var c = countIO(text);
      $("out").textContent = r.blif;
      $("out").className = "code";
      $("copy").disabled = false;
      $("s-luts").textContent = r.luts;
      $("s-depth").innerHTML = r.depth + '<span class="unit">levels</span>';
      $("s-pi").textContent = c.pi;
      $("s-po").textContent = c.po;
      $("s-k").textContent = k;
      setStatus("Mapped in " + ms + " ms", "ok");
    }
    busy = false;
    $("run").disabled = false; $("sweep").disabled = false;
  }

  async function sweep() {
    if (!api || busy) return;
    var text = $("src").value;
    if (!text.trim()) { setStatus("Nothing to sweep.", "err"); return; }

    busy = true;
    $("run").disabled = true; $("sweep").disabled = true;
    var body = $("sweep-body");
    var ks = [2, 3, 4, 5, 6, 7, 8];
    var rows = {};
    body.innerHTML = "";
    ks.forEach(function (k) {
      var tr = document.createElement("tr");
      tr.className = "pending";
      tr.innerHTML = '<td>' + k + '</td><td class="n">·</td><td>·</td><td class="bar"></td>';
      body.appendChild(tr);
      rows[k] = tr;
    });

    var results = [];
    for (var i = 0; i < ks.length; i++) {
      var k = ks[i];
      setStatus("Sweeping… k = " + k);
      await nextFrame();
      var r = mapOnce(text, k);
      if (r.error) { setStatus(r.error, "err"); break; }
      results.push({ k: k, luts: r.luts, depth: r.depth });
      rows[k].className = "";
      rows[k].children[1].textContent = r.luts;
      rows[k].children[2].textContent = r.depth;
      var max = Math.max.apply(null, results.map(function (x) { return x.luts; }));
      results.forEach(function (x) {
        var bar = rows[x.k].children[3];
        bar.innerHTML = '<div class="barline" style="width:' + Math.max(2, (x.luts / max) * 100) + '%"></div>';
      });
    }

    if (results.length) {
      var best = results.reduce(function (a, b) { return b.luts < a.luts ? b : a; });
      rows[best.k].classList.add("best");
      setStatus("Sweep complete — fewest LUTs at k = " + best.k, "ok");
    }
    busy = false;
    $("run").disabled = false; $("sweep").disabled = false;
  }

  function pickSample(s) {
    $("src").value = sampleText(s.key);
    refreshInMeta();
    Array.prototype.forEach.call(document.querySelectorAll("#samples .chip"), function (c) {
      c.setAttribute("aria-pressed", String(c.dataset.id === s.id));
    });
  }

  function buildChips() {
    var host = $("samples");
    SAMPLES.forEach(function (s) {
      var b = document.createElement("button");
      b.type = "button"; b.className = "chip"; b.dataset.id = s.id;
      b.setAttribute("aria-pressed", "false");
      b.innerHTML = s.name + '<small>' + s.note + '</small>';
      b.addEventListener("click", function () { pickSample(s); run(); });
      host.appendChild(b);
    });
  }

  function wire() {
    $("run").addEventListener("click", run);
    $("sweep").addEventListener("click", sweep);
    $("k").addEventListener("change", function () { if (api) run(); });
    $("src").addEventListener("input", function () {
      refreshInMeta();
      Array.prototype.forEach.call(document.querySelectorAll("#samples .chip"), function (c) {
        c.setAttribute("aria-pressed", "false");
      });
    });
    $("file").addEventListener("change", function (e) {
      var f = e.target.files && e.target.files[0];
      if (!f) return;
      var fr = new FileReader();
      fr.onload = function () { $("src").value = String(fr.result); refreshInMeta(); run(); };
      fr.readAsText(f);
      e.target.value = "";
    });
    $("copy").addEventListener("click", function () {
      var t = $("out").textContent, btn = $("copy");
      var done = function () { btn.textContent = "Copied"; setTimeout(function () { btn.textContent = "Copy"; }, 1400); };
      if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(t).then(done, function () { btn.textContent = "Select the text to copy"; });
      } else {
        var r = document.createRange(); r.selectNodeContents($("out"));
        var sel = window.getSelection(); sel.removeAllRanges(); sel.addRange(r); done();
      }
    });
  }

  buildChips();
  wire();
  pickSample(SAMPLES[0]);

  createLutmap().then(function (M) {
    api = {
      map: M.cwrap("lutmap_map", "string", ["string", "number"]),
      err: M.cwrap("lutmap_error", "string", []),
      luts: M.cwrap("lutmap_luts", "number", []),
      depth: M.cwrap("lutmap_depth", "number", [])
    };
    run();
  }, function (e) {
    setStatus("Could not start the WebAssembly mapper in this browser.", "err");
    console.error(e);
  });
})();

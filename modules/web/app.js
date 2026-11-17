/* Browser front end.
 * Talks to the C server with fetch():
 *   POST /api/recursion  {algorithm, input}
 *   POST /api/sort       {algorithm, input:[..]}
 *   POST /api/benchmark  {family, sizes:[..]}
 * Every response carries operations/comparisons/swaps, running_time_ms
 * and a steps array that we render as a collapsible trace.
 */

const API = {
  recursion: "/api/recursion",
  sorting: "/api/sort",
  benchmark: "/api/benchmark"
};

const structureState = { stack: [], queue: [], circular: [], singly: [], doubly: [] };
const STRUCTURE_CAPACITY = 8;

function structureHtml(name, message) {
  const values = structureState[name];
  const direction = name === "stack" ? "bottom → top" : "front → rear";
  return "<h3>" + name + "</h3><p>" + message + "</p><p><b>" + direction + ":</b> " +
    (values.length ? values.join(" → ") : "empty") + "</p><p>size: " + values.length +
    (name === "stack" || name === "queue" || name === "circular" ? " / " + STRUCTURE_CAPACITY : "") + "</p>";
}

document.getElementById("form-structures").addEventListener("submit", function (ev) {
  ev.preventDefault();
  const form = ev.target, name = form.structure.value, op = form.operation.value;
  const values = structureState[name], value = Number(form.value.value);
  let message = "";
  if (op === "reset") { values.length = 0; message = "Structure reset."; }
  else if (op === "insert_end" || op === "insert_front") {
    if ((name === "stack" || name === "queue" || name === "circular") && values.length >= STRUCTURE_CAPACITY) message = "Overflow: fixed capacity reached.";
    else { op === "insert_front" ? values.unshift(value) : values.push(value); message = "Inserted " + value + "."; }
  } else if (op === "remove_front") {
    if (!values.length) message = "Underflow: structure is empty.";
    else message = "Removed " + (name === "stack" ? values.pop() : values.shift()) + ".";
  } else if (op === "remove_end") {
    message = values.length ? "Removed " + values.pop() + "." : "Underflow: structure is empty.";
  } else if (op === "peek") {
    message = values.length ? "Current value: " + (name === "stack" ? values[values.length - 1] : values[0]) + "." : "Structure is empty.";
  } else if (op === "search") {
    const index = values.indexOf(value); message = index < 0 ? value + " was not found." : value + " found at position " + index + ".";
  } else if (op === "reverse") { values.reverse(); message = "Display order reversed."; }
  showResult("structures-result", structureHtml(name, message));
});

document.getElementById("form-brackets").addEventListener("submit", function (ev) {
  ev.preventDefault(); const expression = ev.target.expression.value, stack = [], pairs = {")": "(", "]": "[", "}": "{"};
  let valid = true;
  for (const ch of expression) { if ("([{".includes(ch)) stack.push(ch); else if (pairs[ch] && stack.pop() !== pairs[ch]) { valid = false; break; } }
  valid = valid && stack.length === 0;
  showResult("brackets-result", "<p class='" + (valid ? "working" : "error") + "'>" + (valid ? "Balanced expression." : "Not balanced.") + "</p>");
});

function showResult(id, html) {
  document.getElementById(id).innerHTML = html;
}

function metricsHtml(data) {
  const parts = [];
  if (data.operations !== undefined) {
    parts.push("operations: " + data.operations);
  }
  if (data.comparisons !== undefined) {
    parts.push("comparisons: " + data.comparisons);
  }
  if (data.swaps !== undefined) {
    parts.push("swaps: " + data.swaps);
  }
  if (data.writes !== undefined) {
    parts.push("writes/moves: " + data.writes);
  }
  if (data.time_ms !== undefined) {
    parts.push("time: " + data.time_ms + " ms");
  } else if (data.running_time_ms !== undefined) {
    parts.push("time: " + data.running_time_ms + " ms");
  }
  return parts.length ? '<p class="metrics">' + parts.join(" | ") + "</p>" : "";
}

function stepsHtml(data) {
  const steps = data.steps;
  if (!steps || steps.length === 0) {
    return "";
  }
  const escapeHtml = function (s) {
    return String(s).replace(/[&<>\"']/g, function (ch) {
      return ({"&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;", "'": "&#39;"})[ch];
    });
  };
  const items = steps.map(function (s) { return "<li>" + escapeHtml(s) + "</li>"; }).join("");
  let note = "";
  if (data.steps_note) {
    note = "<p class='note'>" + data.steps_note + "</p>";
  }
  return (
    "<details><summary>Show step trace (" + steps.length + " steps)</summary>" +
    note +
    "<ol>" + items + "</ol></details>"
  );
}

async function post(url, body) {
  const response = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body)
  });
  const text = await response.text();
  let data;
  try {
    data = JSON.parse(text);
  } catch (e) {
    throw new Error("server did not answer with JSON: " + text.slice(0, 120));
  }
  if (!response.ok || data.error) {
    throw new Error(data.error || text.slice(0, 120));
  }
  return data;
}

/* Recursion panel */
document.getElementById("form-recursion").addEventListener("submit", async function (ev) {
  ev.preventDefault();
  const form = ev.target;
  const body = {
    algorithm: form.algorithm.value,
    input: Number(form.input.value)
  };
  showResult("recursion-result", "<p class='working'>Running...</p>");
  try {
    const data = await post(API.recursion, body);
    showResult("recursion-result",
      "<h3>" + data.algorithm + "(n=" + data.input + ") = " + data.result + "</h3>" +
      metricsHtml(data) + stepsHtml(data));
  } catch (err) {
    showResult("recursion-result", "<p class='error'>Error: " + err.message + "</p>");
  }
});

/* Sorting panel */
document.getElementById("form-sorting").addEventListener("submit", async function (ev) {
  ev.preventDefault();
  const form = ev.target;
  const numbers = form.input.value.split(",").map(Number);
  if (numbers.length === 0 || numbers.some(isNaN)) {
    showResult("sorting-result", "<p class='error'>Enter comma separated numbers.</p>");
    return;
  }
  if (numbers.length > 10000) {
    showResult("sorting-result", "<p class='error'>Max 10000 numbers.</p>");
    return;
  }
  showResult("sorting-result", "<p class='working'>Sorting...</p>");
  try {
    const data = await post(API.sorting, {
      algorithm: form.algorithm.value,
      input: numbers
    });
    showResult("sorting-result",
      "<h3>" + data.algorithm + " sorted " + data.size + " numbers</h3>" +
      "<p class='sorted-list'>" + data.sorted.join(", ") + "</p>" +
      metricsHtml(data) + stepsHtml(data));
  } catch (err) {
    showResult("sorting-result", "<p class='error'>Error: " + err.message + "</p>");
  }
});

/* Benchmark panel */
document.getElementById("form-benchmark").addEventListener("submit", async function (ev) {
  ev.preventDefault();
  const form = ev.target;
  const sizes = form.sizes.value.split(",").map(Number).filter(function (n) {
    return !isNaN(n) && n > 0;
  });
  if (sizes.length === 0) {
    showResult("benchmark-result", "<p class='error'>Enter sizes like 100,200,400.</p>");
    return;
  }
  showResult("benchmark-result", "<p class='working'>Benchmarking, this can take a few seconds...</p>");
  try {
    const data = await post(API.benchmark, {
      family: form.family.value,
      sizes: sizes,
      input_type: form.input_type.value,
      repeats: Number(form.repeats.value)
    });
    let html = "<p>Input: " + data.input_type + " | average of " + data.repeats + " runs</p>" +
               "<table><tr><th>algorithm</th><th>size</th><th>operations</th>" +
               "<th>comparisons</th><th>swaps</th><th>writes/moves</th><th>time (ms)</th></tr>";
    for (const row of data.rows) {
      html += "<tr><td>" + row.algorithm + "</td><td>" + row.size + "</td>" +
              "<td>" + (row.operations || "") + "</td>" +
              "<td>" + (row.comparisons === undefined ? "-" : row.comparisons) + "</td>" +
              "<td>" + (row.swaps === undefined ? "-" : row.swaps) + "</td>" +
               "<td>" + (row.writes === undefined ? "-" : row.writes) + "</td>" +
              "<td>" + row.time_ms + "</td></tr>";
    }
    html += "</table>";
    showResult("benchmark-result", html);
  } catch (err) {
    showResult("benchmark-result", "<p class='error'>Error: " + err.message + "</p>");
  }
});

/* Panel switching */
document.querySelectorAll("nav button").forEach(function (btn) {
  btn.addEventListener("click", function () {
    document.querySelectorAll("nav button").forEach(function (b) {
      b.classList.remove("active");
    });
    document.querySelectorAll(".panel").forEach(function (p) {
      p.classList.remove("active");
    });
    btn.classList.add("active");
    document.getElementById("panel-" + btn.dataset.panel).classList.add("active");
  });
});

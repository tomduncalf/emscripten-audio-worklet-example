const express = require("express");
const path = require("path");
const app = express();
const port = 8000;

// Set CORS headers
app.use((req, res, next) => {
  res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
  res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
  res.setHeader(
    "Cache-Control",
    "no-store, no-cache, must-revalidate, proxy-revalidate"
  );
  res.setHeader("Pragma", "no-cache");
  res.setHeader("Expires", "0");

  next();
});

// Serve static files from current directory
app.use(express.static("../"));

app.listen(port, () => {
  console.log(`Server running at http://localhost:${port}`);
});

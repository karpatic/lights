const fs = require('fs');
const path = require('path');

const root = __dirname;
const indexPath = path.join(root, 'index.html');
const errors = [];

function readText(relativePath) {
  return fs.readFileSync(path.join(root, relativePath), 'utf8');
}

function fail(message) {
  errors.push(message);
}

function fileExists(relativePath) {
  return fs.existsSync(path.join(root, relativePath));
}

function parseJsonScript(html, id) {
  const pattern = new RegExp(`<script[^>]+id=["']${id}["'][^>]*>([\\s\\S]*?)<\\/script>`);
  const match = html.match(pattern);
  if (!match) {
    fail(`Missing JSON script #${id}`);
    return null;
  }

  try {
    return JSON.parse(match[1]);
  } catch (error) {
    fail(`Invalid JSON script #${id}: ${error.message}`);
    return null;
  }
}

function normalizeRoutePath(value, aliases) {
  if (!value || value === '#') {
    return '/';
  }

  let normalized = String(value).trim();
  if (normalized.startsWith('#')) {
    normalized = normalized.slice(1);
  }

  if (!normalized || normalized === '/') {
    return '/';
  }

  if (!normalized.startsWith('/')) {
    normalized = `/${normalized}`;
  }

  if (!normalized.includes('.')) {
    normalized = `${normalized}.html`;
  }

  return aliases[normalized] || normalized;
}

function validateLocalTarget(rawHref, source, routePaths, aliases) {
  const href = rawHref.trim();
  if (
    !href ||
    href.startsWith('http://') ||
    href.startsWith('https://') ||
    href.startsWith('mailto:') ||
    href.startsWith('tel:')
  ) {
    return;
  }

  if (href.startsWith('#')) {
    const route = normalizeRoutePath(href, aliases);
    if (!routePaths.has(route)) {
      fail(`${source}: unknown route link ${href}`);
    }
    return;
  }

  const withoutFragment = href.split('#')[0].split('?')[0];
  if (!withoutFragment) {
    return;
  }

  const rooted = withoutFragment.startsWith('/')
    ? withoutFragment.slice(1)
    : withoutFragment.replace(/^\.\//, '');
  const resolved = path.normalize(rooted);

  if (resolved.startsWith('..')) {
    fail(`${source}: link escapes repository root: ${href}`);
    return;
  }

  if (resolved.endsWith('.html')) {
    const route = normalizeRoutePath(`/${resolved}`, aliases);
    if (routePaths.has(route)) {
      return;
    }
  }

  if (!fileExists(resolved)) {
    fail(`${source}: missing local target ${href} -> ${resolved}`);
  }
}

function extractMarkdownLinks(text) {
  const links = [];
  const markdownLink = /!?\[[^\]]*]\(([^)\s]+)(?:\s+["'][^"']*["'])?\)/g;
  let match;

  while ((match = markdownLink.exec(text))) {
    links.push(match[1]);
  }

  return links;
}

function extractHtmlReferences(text) {
  const links = [];
  const attr = /\b(?:href|src)=["']([^"']+)["']/g;
  let match;

  while ((match = attr.exec(text))) {
    links.push(match[1]);
  }

  return links;
}

const indexHtml = readText('index.html');
const routeMap = parseJsonScript(indexHtml, 'route-map') || [];
const aliases = parseJsonScript(indexHtml, 'route-aliases') || {};
const routePaths = new Set(routeMap.map((route) => route.path));

if (!routePaths.has('/')) {
  fail('Route map must include /');
}

for (const route of routeMap) {
  if (!route.path || !route.notebook || !route.label) {
    fail(`Incomplete route entry: ${JSON.stringify(route)}`);
    continue;
  }

  if (route.notebook.includes('archive/')) {
    fail(`Archived notebook appears in active navigation: ${route.notebook}`);
  }

  const notebookPath = route.notebook.replace(/^\.\//, '');
  if (!fileExists(notebookPath)) {
    fail(`${route.path}: missing notebook ${route.notebook}`);
    continue;
  }

  let notebook;
  try {
    notebook = JSON.parse(readText(notebookPath));
  } catch (error) {
    fail(`${route.notebook}: invalid notebook JSON: ${error.message}`);
    continue;
  }

  if (!Array.isArray(notebook.cells)) {
    fail(`${route.notebook}: missing cells array`);
    continue;
  }

  const markdown = notebook.cells
    .filter((cell) => cell.cell_type === 'markdown')
    .flatMap((cell) => Array.isArray(cell.source) ? cell.source : [cell.source || ''])
    .join('');

  for (const link of [...extractMarkdownLinks(markdown), ...extractHtmlReferences(markdown)]) {
    validateLocalTarget(link, route.notebook, routePaths, aliases);
  }
}

for (const [alias, target] of Object.entries(aliases)) {
  if (!routePaths.has(target)) {
    fail(`Alias ${alias} targets missing route ${target}`);
  }
}

for (const required of ['PROJECT_STATE.md', 'README.md', 'bt.html', 'lights.js', 'upload.html', 'web.js']) {
  if (!fileExists(required)) {
    fail(`Missing required file ${required}`);
  }
}

if (!routePaths.has('/wled.html')) {
  fail('Restored notebook route map must include /wled.html');
}

if (!routePaths.has('/upload.html')) {
  fail('Restored notebook route map must include /upload.html');
}

if (errors.length) {
  console.error('Site validation failed:');
  for (const error of errors) {
    console.error(`- ${error}`);
  }
  process.exit(1);
}

console.log(`Validated ${routeMap.length} active notebook routes.`);

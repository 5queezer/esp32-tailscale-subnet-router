#!/usr/bin/env node
/**
 * Exercise the production snapshotNetListFromDOM function from index.html.
 *
 * Usage:
 *   node tools/tests/test_dns_ui.js
 *
 * The function is extracted and evaluated with a minimal DOM mock so the test
 * follows the UI implementation instead of maintaining a copied function.
 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

function extractFunction(source, signature) {
  const start = source.indexOf(signature);
  assert.notEqual(start, -1, `production function ${signature} was not found`);
  const opening = source.indexOf('{', start);
  assert.notEqual(opening, -1, `production function ${signature} has no body`);

  let depth = 0;
  for (let index = opening; index < source.length; index += 1) {
    if (source[index] === '{') depth += 1;
    if (source[index] === '}') {
      depth -= 1;
      if (depth === 0) return source.slice(start, index + 1);
    }
  }
  assert.fail(`production function ${signature} has an incomplete body`);
}

const root = path.resolve(__dirname, '..', '..');
const html = fs.readFileSync(path.join(root, 'main', 'index.html'), 'utf8');
const productionFunction = extractFunction(
  html,
  'function snapshotNetListFromDOM()'
);

const network = { ssid: 'example-network', static_ip: { dns: '192.0.2.53' } };
const fields = {
  '.net-ssid': { value: 'example-network' },
  '.net-pw': { value: '' },
  '.net-static-toggle': { checked: false },
  '.net-dns': { value: '192.0.2.53' },
  '.net-eap-method': { value: '0' },
  '.net-ip': { value: '198.51.100.20' },
  '.net-mask': { value: '255.255.255.0' },
  '.net-gw': { value: '198.51.100.1' }
};
const row = {
  dataset: { idx: '0' },
  querySelector: (selector) => fields[selector]
};
const context = {
  document: { querySelectorAll: () => [row] },
  s_net_list: [network]
};

vm.createContext(context);
vm.runInContext(productionFunction, context);
const plain = (value) => JSON.parse(JSON.stringify(value));

context.snapshotNetListFromDOM();
assert.deepEqual(plain(network.static_ip), { dns: '192.0.2.53' });

fields['.net-static-toggle'].checked = true;
context.snapshotNetListFromDOM();
assert.deepEqual(plain(network.static_ip), {
  ip: '198.51.100.20',
  mask: '255.255.255.0',
  gw: '198.51.100.1',
  dns: '192.0.2.53'
});

fields['.net-static-toggle'].checked = false;
fields['.net-dns'].value = '';
context.snapshotNetListFromDOM();
assert.deepEqual(plain(network.static_ip), { dns: '' });

console.log('PASS DHCP DNS persists, static DNS persists, and clearing persists');

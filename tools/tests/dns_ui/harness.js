/* Serialization-only fixture: the mask sentinel is deliberately not an IP.
 * Validation belongs to the form validators, not snapshotNetListFromDOM.
 */
const assert = require('node:assert/strict');

const network = { ssid: 'example-network', static_ip: { dns: '192.0.2.53' } };
const fields = {
  '.net-ssid': { value: 'example-network' },
  '.net-pw': { value: '' },
  '.net-static-toggle': { checked: false },
  '.net-dns': { value: '192.0.2.53' },
  '.net-eap-method': { value: '0' },
  '.net-ip': { value: '198.51.100.20' },
  '.net-mask': { value: 'mask-from-form' },
  '.net-gw': { value: '198.51.100.1' }
};
const row = {
  dataset: { idx: '0' },
  querySelector: (selector) => fields[selector]
};
const document = { querySelectorAll: () => [row] };
const s_net_list = [network];

snapshotNetListFromDOM();
assert.deepEqual(network.static_ip, { dns: '192.0.2.53' });

fields['.net-static-toggle'].checked = true;
snapshotNetListFromDOM();
assert.deepEqual(network.static_ip, {
  ip: '198.51.100.20',
  mask: 'mask-from-form',
  gw: '198.51.100.1',
  dns: '192.0.2.53'
});

fields['.net-static-toggle'].checked = false;
fields['.net-dns'].value = '';
snapshotNetListFromDOM();
assert.deepEqual(network.static_ip, { dns: '' });

console.log('PASS DHCP DNS persists, static DNS persists, and clearing persists');

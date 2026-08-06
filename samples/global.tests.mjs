// test_globalthis_dynamic.mjs
import console from 'console';
globalThis.foo = 'bar';
const mod = await import('./test_module.mjs');
console.log(globalThis.foo);
console.log(mod.hello);

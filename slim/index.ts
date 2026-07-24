import console from 'console';
import server from 'http';
console.trace("begin");
const config: SlimHttpServerConfig = { host: '0.0.0.0', port: 9000 };
async function serve(config: SlimHttpServerConfig) {
    console.debug(config);
    const listener = server.serve(config);
    console.debug("listener created");
    for await (const event of listener) {
        console.debug("event found");
        console.debug(event.request.url);
     }
}
serve(config);
console.trace("end");

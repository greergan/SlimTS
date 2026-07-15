import * as interfaces from 'SlimInterfaces';
import * as utilities from 'SlimUtilities';
import { http } from './http_server_response_codes.ts';
export default class HttpRouter {
    private host:string | undefined;
    private port:number | undefined;
    private rootDirectory:string | undefined;
    private routes:Map<string, interfaces.HttpRoute> = new Map<string, interfaces.HttpRoute>();
    private runningOnMessage:string | undefined;
    private runningSince:number = 0;
    private staticRoutes:Map<string, interfaces.HttpRoute> = new Map<string, interfaces.HttpRoute>();
    private webSocketRoutes:Map<string, interfaces.WebSocketRoute> = new Map<string, interfaces.WebSocketRoute>();
    private middleWareMap:Map<string, interfaces.MiddleWareFunction[]> = new Map<string, interfaces.MiddleWareFunction[]>();
    constructor(configuration:interfaces.HttpRouterConfiguration) {
        this.port = configuration.port ? configuration.port : 8080;
        this.host = configuration.host ? configuration.host.replace(/\s/g, "") : "127.0.0.1";
        this.rootDirectory = configuration.rootDirectory ? get_normalized_uri(configuration.rootDirectory) : get_normalized_uri('.');
        this.runningOnMessage = configuration.runningOnMessage  ? configuration.runningOnMessage : `instance of HttpRouter running => http://${this.host}:${this.port}`;
        console.info({message:"runningOnMessage"}, this.runningOnMessage);
    }
    public addMiddleWare(middleWare:interfaces.MiddleWareFunction) {
        console.debug({message:"beginning with",value:"middleWare"}, middleWare);
        this.middleWareMap.has(middleWare.serverMethod)
            ? this.middleWareMap.get(middleWare.serverMethod)?.push(middleWare)
            : this.middleWareMap.set(middleWare.serverMethod, [middleWare]);
        console.trace();
    }
    public async addRoute(route:interfaces.WebSocketRoute|interfaces.HttpRoute) {
        if(!route.protocol) route.protocol = 'http';
        if(!route.hits) route.hits = 0;
        if(route.protocol == 'http') {
            if(route.uri) route.uri = utilities.get_normalized_uri(route.uri);
            if(!route.rootDirectory && this.rootDirectory) route.rootDirectory = this.rootDirectory;
            else if(!route.uri) throw new Error("addRoute requires one of route.rootDirectory or configuration level rootDirectory");
        }
        console.debug({message:"beginning",value:"with"}, route);
        if(route.protocol == 'webSocket') {
            if(route.uri && typeof route.onMessage === 'function' && typeof route.resolver === 'function') {
                route.socketTuples = [];
                this.webSocketRoutes.set(route.uri, route);
                console.debug({message:"adding", value:"webSocket Route"}, route.uri);
                console.trace({message:"added webSocket Route", value:route.uri}, route);
            }
            else {
                console.error({message:"Required elements", value:"not found"}, route);
            }
        }
        else if(route.resolver !== 'static') {
            // rendered routes
            if(route.inputFile && this.rootDirectory) {
                route.normalizedUrl = await utilities.get_normalized_url(`${route.rootDirectory}/${route.inputFile}`);
            }
            else {
                // static file routes
                if(route.resolver.startsWith('/')) route.resolver = route.resolver.substring(1);
                route.normalizedUrl = await utilities.get_normalized_url(`${route.rootDirectory}/${route.resolver}`);
            }
            console.debug({message:"normalizedUrl",value:route.normalizedUrl});
            if(route.normalizedUrl) {
                route.contentType = route.contentType ? route.contentType : utilities.get_content_type(route.normalizedUrl);
            }
            if(route.contentType && route.uri) {
                this.routes.set(route.uri, route);
                console.trace({message:"added", value:route.uri}, route.resolver);
            }
            else {
                console.warn({message:"unable to find content type", value:"route not added"}, route);
            }
        }
        else if(typeof route.resolver == 'string' && route.resolver == 'static' && route.uri) {
            console.debug({message:"adding",value:"http Route"}, route.uri);
            this.staticRoutes.set(route.uri, route);
            console.trace({message:"added", value:route.uri}, route.resolver);
        }
        else if(typeof route.resolver == 'function') {
            console.debug({message:"adding",value:"http Route with function resolver"}, route.uri);
            this.routes.set(route.uri, route);
            console.trace({message:"added", value:route.uri}, route);
        }
        if(this.middleWareMap.has('addRoute')) {
            for await(const middleWare of (this.middleWareMap.get('addRoute'))!) {
                await middleWare.function(route);
                console.trace({message:"middleWare", value:"function called"}, route.uri);
            }
        }
        console.trace();
    }
    public getRoute(uri:string, protocol:string='http'):interfaces.HttpRoute|interfaces.WebSocketRoute|undefined {
        console.debug({message:"beginning with"}, uri, protocol);
        if(protocol.toLowerCase() == 'websocket') {
            console.trace({message:"returning", value:this.webSocketRoutes.get(uri)})
            return this.webSocketRoutes.get(uri) as interfaces.WebSocketRoute;
        }
        else {
            console.trace({message:"returning", value:this.routes.get(uri)})
            return this.routes.get(uri) as interfaces.HttpRoute;
        }
    }
    public getRoutes(protocol:'http'|'webSocket'|'all'): slim.types.iKeyValueAny | slim.types.iKeyValueAny[] {
        console.debug({message:"beginning with", value:protocol});
        if(protocol.toLowerCase() == 'all') {
            console.trace({message:"returning", value:protocol.toLowerCase()});
            const return_http_routes:slim.types.iKeyValueAny[] = [];
            const return_websocket_routes:slim.types.iKeyValueAny[] = [];
            for(const route of this.routes) {
                return_http_routes.push(route[1]);
            }
            for(const route of this.webSocketRoutes) {
                return_websocket_routes.push(route[1]);
            }
            return {
                http: return_http_routes,
                websocket: return_websocket_routes
            };
        }
        else if(protocol.toLowerCase() == 'http') {
            const return_routes:slim.types.iKeyValueAny[] = [];
            for(const route of this.routes) {
                const return_route = utilities.copy_ofSync(route[1]);
                if(typeof route[1].resolver === 'function') return_route.resolver = route[1].resolver.toString();
                return_routes.push(return_route);
            }
            console.trace({message:"returning", value:protocol.toLowerCase()}, return_routes.length);
            return return_routes;
        }
        else if(protocol.toLowerCase() == 'websocket') {
            const return_routes:slim.types.iKeyValueAny[] = [];
            for(const route of this.webSocketRoutes) {
                return_routes.push(route[1]);
            }
            console.trace({message:"returning", value:protocol.toLowerCase()}, return_routes.length);
            return return_routes;
        }
    }
    public get ServerStartedTime() {
        console.trace();
        return this.runningSince;
    }
    private async connectionHandler(connection:Deno.Conn) {
        const httpConnection:Deno.HttpConn = Deno.serveHttp(connection);
        /*
         * httpConn.localAddr
         * httpConn.remoteAddr
         * httpConn.rid
         * httpConn.readable
         * httpConn.writable
         */
        for await(const requestEvent of httpConnection) {
            console.debug({message:"Request event"}, httpConnection.localAddr, httpConnection.remoteAddr, httpConnection.rid, new URL(requestEvent.request.url).pathname);
            if(this.configuration.handleWebsockets
                && requestEvent.request.headers.get("Connection") == 'Upgrade' && requestEvent.request.headers.get("Upgrade") == 'websocket') {
                    console.debug({message:"Request event",value:"webSocket"}, httpConnection.rid, new URL(requestEvent.request.url).pathname);
                    await this.webSocketRequestHandler(requestEvent);
            }
            else {
                console.debug({message:"Request event",value:"http"}, httpConnection.rid, new URL(requestEvent.request.url).pathname);
                await this.requestHandler(requestEvent);
            }
        }
        console.trace(httpConnection.rid);
    }
    private notFoundHandler(requestEvent:Deno.RequestEvent) {
        requestEvent.respondWith(new Response(null, { status: http.NotFound }));
        console.trace({message:"Responded with", value:"not found"}, requestEvent.request.url);
    }
    private async respondHTTP(content:Deno.JSON|Deno.Blob|string, route:interfaces.HttpRoute, requestEvent:Deno.RequestEvent) {
        const status = content !== 'undefined' ? http.OK : http.NotFound;
        requestEvent.respondWith(new Response(content, {
            status: status, headers: { "content-type": route.contentType! }
        }));
        console.trace({message:"Responded with", value:status}, route.contentType, route.uri);
    }
    private async requestHandler(requestEvent:Deno.RequestEvent) {
        const url = new URL(utilities.get_normalized_uri(requestEvent.request.url));
        console.debug({message:"requestEvent for", value:url.pathname});
        let route:interfaces.HttpRoute|undefined = this.routes.get(url.pathname);
        if(!route) {
            for(const static_route of this.staticRoutes) {
                console.debug({message:"comparing", value:"route"}, static_route[1].uri, url.pathname);
                if(static_route[1].uri == url.pathname.substring(0, static_route[1].uri.length)) {
                    const resolver_string:string = (static_route[1].uri == url.pathname) ? `index.html` : url.pathname.substring(static_route[1].uri.length);
                    console.debug({message:"adding static route", value:resolver_string});
                    await this.addRoute({uri:url.pathname,protocol:"http",resolver:resolver_string,hits:0,discovered:true,
                        url:requestEvent.request.url, rootDirectory:static_route[1].rootDirectory});
                    route = this.routes.get(url.pathname);
                }
            }
        }
        console.debug({message:"route"}, route?.uri);
        if(route) {
            if(!route.url) route.url = requestEvent.request.url;
            if(!route.discovered) route.discovered = false;
            if('hits' in route) route.hits++;
            console.debug({message:"route should have URL now", value:route.url})
            if(this.middleWareMap.has('requestHandler')) {
                for await(const middleWare of (this.middleWareMap.get('requestHandler'))!) {
                    await middleWare.function(route);
                    console.trace({message:"middleWare", value:"function called"}, route.uri);
                }
            }
            if(typeof route.resolver === 'string' && utilities.is_valid_url(route.normalizedUrl) && route.contentType) {
                if(route.contentType == "text/json") {
                    this.respondHTTP(await utilities.get_json_contents(route.normalizedUrl), route, requestEvent);
                }
                else if(utilities.is_text_content_type(route.contentType)) {
                    this.respondHTTP(await utilities.get_file_contents(route.normalizedUrl), route, requestEvent);
                }
                else if(utilities.is_binary_content_type(route.contentType)) {
                    this.respondHTTP(await utilities.get_binary_contents(route.normalizedUrl), route, requestEvent);
                }
                else {
                    requestEvent.respondWith(new Response(null, { status: http.InternalServerError }));
                }
            }
            else if(typeof route.resolver === 'function' && route.contentType) {
                this.respondHTTP(await route.resolver(), route, requestEvent);
            }
            else {
                requestEvent.respondWith(new Response(null, { status: http.InternalServerError }));
            }
        }
        else {
            this.notFoundHandler(requestEvent);
        }
        console.trace();
    }
    public async startRouter() {
        this.server = Deno.listen(this.listenOptions);
        this.runningSince = new Date().getTime();
        console.info(this.configuration.runningOnMessage);
        if(this.server) {
            for await (const connection of this.server) {
                this.connectionHandler(connection);
            }
        }
        console.trace();
    }
    private async webSocketRequestHandler(requestEvent:Deno.RequestEvent) {
        const url = new URL(utilities.get_normalized_uri(requestEvent.request.url));
        console.debug({message:"beginning with", value:"url.pathname"}, url.pathname);
        if(this.webSocketRoutes.has(url.pathname)) {
            console.debug({message:"upgrading connection to", value:"WebSocket"}, url.pathname);
            const { socket, response } = Deno.upgradeWebSocket(requestEvent.request);
            const route:interfaces.WebSocketRoute|undefined = this.webSocketRoutes.get(url.pathname);
            if(route) {
                if(!route.url) route.url = requestEvent.request.url;
                if(!route.socketTuples) route.socketTuples = [];
                const onOpenDebug = () => console.debug();
                socket.onopen = (event) => onOpenDebug();
                socket.onopen = route.resolver;
                socket.onmessage = route.onMessage;
                route.socketTuples.push({socket:socket,messagesSent:0});
                console.trace({message:"requestEvent",value:"url"}, url.pathname);
                requestEvent.respondWith(response);
            }
            else {
                console.trace({message:"Websocket",value:"not found"}, url.pathname);
                this.notFoundHandler(requestEvent);
            }
        }
        else {
            console.trace({message:"Websocket",value:"not found"}, url.pathname);
            this.notFoundHandler(requestEvent);
        }
    }
}

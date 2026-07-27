export interface HttpRouterConfiguration {
    port:number,
    host?:string,
    rootDirectory?:string,
    headers?:Headers,
    handleWebsockets?:boolean,
    runningOnMessage?:string
}
export interface HttpRoute {
    hits?:number,
    uri:string,
    url?:string,
    resolver:Function|string|'static',
    inputFile?:string,
    protocol?:'http'|'webSocket'|undefined,
    headers?:Headers,
    contentType?:string|undefined,
    forceCache?:boolean,
    normalizedUrl?:string|undefined,
    rootDirectory?:string,
    discovered?:true|false|'generated'
}
export interface WebSocketTuple {
    socket:WebSocket,
    messagesSent:number
}
export interface WebSocketRoute extends HttpRoute {
    onMessage:Function;
    socketTuples:WebSocketTuple[];
}
export interface MiddleWareFunction {
    serverMethod:string,
    function:Function
}

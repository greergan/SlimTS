declare module 'console' {
    const console: {
        log(...args: any[]): void;
        debug(...args: any[]): void;
        error(...args: any[]): void;
        trace(...args: any[]): void;
    };
    export default console;
}

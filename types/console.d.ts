declare module 'console' {
    const console: {
        log(...args: any[]): void;
        error(...args: any[]): void;
    };
    export default console;
}

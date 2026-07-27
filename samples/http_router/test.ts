import console from 'console';
import HttpRouterConfiguration from 'SlimInterfaces';
import HttpRouter from 'HttpRouter';
console.info("start testing HttpRouter");
const router_configuration:HttpRouterConfiguration = {
	port: 8080,
	host: "	localhost",
	rootDirectory: "./public",
}
const router:HttpRouter = new HttpRouter(router_configuration);
router.start();
//console.debug();
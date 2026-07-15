export type Dictionary_type<K extends string | number | symbol, V> = {
	[key in K]: V;
}
export default {}
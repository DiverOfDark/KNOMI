import { mount } from "svelte";
import App from "./App.svelte";
import "./App.scss";

const target = document.getElementById("app");

if (!target) {
    throw new Error("Target not found");
}

const app = mount(App, { target });
export default app;

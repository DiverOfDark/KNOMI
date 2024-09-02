import eslintPluginSvelte from "eslint-plugin-svelte";
import tseslint from 'typescript-eslint';
export default [
    // add more generic rule sets here, such as:
    // js.configs.recommended,
    ...eslintPluginSvelte.configs["flat/recommended"],
    ...tseslint.configs.recommended,
    {
        rules: {
            // override/add rules settings here, such as:
            // 'svelte/rule-name': 'error'
            semi: "error",
        }
    },
];

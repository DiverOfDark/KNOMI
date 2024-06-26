Import("env")

def generate_webui(*arg, **kwargs):
    # You must have nodejs/npm installed
    env.Execute(f"cd ui-ng && npm install --legacy-peer-deps")
    env.Execute(f"cd ui-ng && npm run lint")
    env.Execute(f"cd ui-ng && npm run format")
    env.Execute(f"cd ui-ng && npm run build-silent")


env.AddCustomTarget(
    "generate_ui",
    None,
    generate_webui,
    title="generate_ui",
    description="Generate npm ui")

propName = "custom_generateUI".lower()
default = env.GetProjectConfig().get("env", propName)

if not BUILD_TARGETS and env.GetProjectOption(propName, default) == "true":
    generate_webui();
import react from "@vitejs/plugin-react";
import tailwindcss from "@tailwindcss/vite";
import { defineConfig } from "vite";
import { fileURLToPath, URL } from "node:url";

export default defineConfig({
  plugins: [
    react(),
    tailwindcss(),
  ],

  resolve: {
    alias: {
      "@shadcn": fileURLToPath(
        new URL("./src/components/ui", import.meta.url)
      ),

      "@components": fileURLToPath(
        new URL("./src/components", import.meta.url)
      ),

      "@navigation": fileURLToPath(
        new URL("./src/navigation", import.meta.url)
      ),

      "@features": fileURLToPath(
        new URL("./src/features", import.meta.url)
      ),

      "@services": fileURLToPath(
        new URL("./src/services", import.meta.url)
      ),

      "@styles": fileURLToPath(
        new URL("./src/styles", import.meta.url)
      ),

      "@models": fileURLToPath(
        new URL("./src/types", import.meta.url)
      ),

      "@hooks": fileURLToPath(
        new URL("./src/hooks", import.meta.url)
      ),

      "@lib": fileURLToPath(
        new URL("./src/lib", import.meta.url)
      ),
    },
  },
});
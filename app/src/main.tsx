import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './index.css'
import { App } from './App.tsx'
import { BluetoothProvider } from '@features/bluetooth/BluetoothProvider.tsx' 

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <BluetoothProvider>
      <App />
    </BluetoothProvider>
  </StrictMode>,
)

import { useRef, useEffect } from 'react'

const useCanvas = (draw, scale = 1) => {
  
    const canvasRef = useRef(null)
  
    useEffect(() => {
    
        const canvas = canvasRef.current
        const context = canvas.getContext('2d')
        let frameCount = 0
        let animationFrameId
    
        const render = () => {
            frameCount++
            draw(context, frameCount)
            animationFrameId = window.requestAnimationFrame(render)
        }
        render()
    
        return () => {
            window.cancelAnimationFrame(animationFrameId)
        }
    }, [draw])

    useEffect(() => {
        const canvas = canvasRef.current
        const context = canvas.getContext('2d')
        context.scale(scale,scale);
        context.imageSmoothingEnabled = false;
        const s = scale;
        return () => {
            context.scale(1/s, 1/s);
        }
    }, [scale])
  
    return canvasRef
}

export default useCanvas    
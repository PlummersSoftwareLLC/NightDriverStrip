import useCanvas from "../../../hook/canvasHook"

const Canvas = ({ draw, scale, ...rest }) => {  
    const canvasRef = useCanvas(draw, scale)
  
    return <canvas ref={canvasRef} {...rest}/>
}

export default Canvas
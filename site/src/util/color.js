const intToRGB = (int) => {
    const bits = int.toString(2).padStart(24, 0);
    return {
        r: parseInt(bits.substring(0, 8), 2),
        g: parseInt(bits.substring(8, 16), 2),
        b: parseInt(bits.substring(16, 24), 2)
    }
}

const RGBToInt = ({r, g, b}) => {
    return parseInt(r.toString(2).padStart(8, 0) + g.toString(2).padStart(8, 0) + b.toString(2).padStart(8, 0), 2);
}

export {intToRGB, RGBToInt}
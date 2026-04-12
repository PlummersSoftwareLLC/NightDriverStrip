import path from 'path';
import { normalizePath } from 'vite';
import fs from 'fs';
import fs$1 from 'fs-extra';
import chalk from 'chalk';
import zlib from 'zlib';
import Debug from 'debug';

const isFunction = (arg) => typeof arg === "function";
const isRegExp = (arg) => Object.prototype.toString.call(arg) === "[object RegExp]";
function readAllFile(root, reg) {
  let resultArr = [];
  try {
    if (fs.existsSync(root)) {
      const stat = fs.lstatSync(root);
      if (stat.isDirectory()) {
        const files = fs.readdirSync(root);
        files.forEach(function(file) {
          const t = readAllFile(path.join(root, "/", file), reg);
          resultArr = resultArr.concat(t);
        });
      } else {
        if (reg !== void 0) {
          if (isFunction(reg.test) && reg.test(root)) {
            resultArr.push(root);
          }
        } else {
          resultArr.push(root);
        }
      }
    }
  } catch (error) {
    throw error;
  }
  return resultArr;
}

const debug = Debug.debug("vite-plugin-compression");
const extRE = /\.(js|mjs|json|css|html)$/i;
const mtimeCache = /* @__PURE__ */ new Map();
function index(options = {}) {
  let outputPath;
  let config;
  const emptyPlugin = {
    name: "vite:compression"
  };
  const {
    disable = false,
    filter = extRE,
    verbose = true,
    threshold = 1025,
    compressionOptions = {},
    deleteOriginFile = false,
    success = () => {
    }
  } = options;
  let { ext = "" } = options;
  const { algorithm = "gzip" } = options;
  if (algorithm === "gzip" && !ext) {
    ext = ".gz";
  }
  if (algorithm === "brotliCompress" && !ext) {
    ext = ".br";
  }
  if (disable) {
    return emptyPlugin;
  }
  debug("plugin options:", options);
  return {
    ...emptyPlugin,
    apply: "build",
    enforce: "post",
    configResolved(resolvedConfig) {
      config = resolvedConfig;
      outputPath = path.isAbsolute(config.build.outDir) ? config.build.outDir : path.join(config.root, config.build.outDir);
      debug("resolvedConfig:", resolvedConfig);
    },
    async closeBundle() {
      let files = readAllFile(outputPath) || [];
      debug("files:", files);
      if (!files.length)
        return;
      files = filterFiles(files, filter);
      const compressOptions = getCompressionOptions(algorithm, compressionOptions);
      const compressMap = /* @__PURE__ */ new Map();
      const handles = files.map(async (filePath) => {
        const { mtimeMs, size: oldSize } = await fs$1.stat(filePath);
        if (mtimeMs <= (mtimeCache.get(filePath) || 0) || oldSize < threshold)
          return;
        let content = await fs$1.readFile(filePath);
        if (deleteOriginFile) {
          fs$1.remove(filePath);
        }
        try {
          content = await compress(content, algorithm, compressOptions);
        } catch (error) {
          config.logger.error("compress error:" + filePath);
        }
        const size = content.byteLength;
        const cname = getOutputFileName(filePath, ext);
        compressMap.set(filePath, {
          size: size / 1024,
          oldSize: oldSize / 1024,
          cname
        });
        await fs$1.writeFile(cname, content);
        mtimeCache.set(filePath, Date.now());
      });
      return Promise.all(handles).then(() => {
        if (verbose) {
          handleOutputLogger(config, compressMap, algorithm);
          success();
        }
      });
    }
  };
}
function filterFiles(files, filter) {
  if (filter) {
    const isRe = isRegExp(filter);
    const isFn = isFunction(filter);
    files = files.filter((file) => {
      if (isRe) {
        return filter.test(file);
      }
      if (isFn) {
        return filter(file);
      }
      return true;
    });
  }
  return files;
}
function getCompressionOptions(algorithm = "", compressionOptions = {}) {
  const defaultOptions = {
    gzip: {
      level: zlib.constants.Z_BEST_COMPRESSION
    },
    deflate: {
      level: zlib.constants.Z_BEST_COMPRESSION
    },
    deflateRaw: {
      level: zlib.constants.Z_BEST_COMPRESSION
    },
    brotliCompress: {
      params: {
        [zlib.constants.BROTLI_PARAM_QUALITY]: zlib.constants.BROTLI_MAX_QUALITY,
        [zlib.constants.BROTLI_PARAM_MODE]: zlib.constants.BROTLI_MODE_TEXT
      }
    }
  };
  return {
    ...defaultOptions[algorithm],
    ...compressionOptions
  };
}
function compress(content, algorithm, options = {}) {
  return new Promise((resolve, reject) => {
    zlib[algorithm](content, options, (err, result) => err ? reject(err) : resolve(result));
  });
}
function getOutputFileName(filepath, ext) {
  const compressExt = ext.startsWith(".") ? ext : `.${ext}`;
  return `${filepath}${compressExt}`;
}
function handleOutputLogger(config, compressMap, algorithm) {
  config.logger.info(`
${chalk.cyan("\u2728 [vite-plugin-compression]:algorithm=" + algorithm)} - compressed file successfully: `);
  const keyLengths = Array.from(compressMap.keys(), (name) => name.length);
  const maxKeyLength = Math.max(...keyLengths);
  compressMap.forEach((value, name) => {
    const { size, oldSize, cname } = value;
    const rName = normalizePath(cname).replace(normalizePath(`${config.build.outDir}/`), "");
    const sizeStr = `${oldSize.toFixed(2)}kb / ${algorithm}: ${size.toFixed(2)}kb`;
    config.logger.info(chalk.dim(path.basename(config.build.outDir) + "/") + chalk.blueBright(rName) + " ".repeat(2 + maxKeyLength - name.length) + " " + chalk.dim(sizeStr));
  });
  config.logger.info("\n");
}

export { index as default };

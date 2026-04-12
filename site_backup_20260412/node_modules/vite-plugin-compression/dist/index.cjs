'use strict';

const path = require('path');
const vite = require('vite');
const fs = require('fs');
const fs$1 = require('fs-extra');
const chalk = require('chalk');
const zlib = require('zlib');
const Debug = require('debug');

function _interopDefaultLegacy (e) { return e && typeof e === 'object' && 'default' in e ? e["default"] : e; }

const path__default = /*#__PURE__*/_interopDefaultLegacy(path);
const fs__default = /*#__PURE__*/_interopDefaultLegacy(fs);
const fs__default$1 = /*#__PURE__*/_interopDefaultLegacy(fs$1);
const chalk__default = /*#__PURE__*/_interopDefaultLegacy(chalk);
const zlib__default = /*#__PURE__*/_interopDefaultLegacy(zlib);
const Debug__default = /*#__PURE__*/_interopDefaultLegacy(Debug);

const isFunction = (arg) => typeof arg === "function";
const isRegExp = (arg) => Object.prototype.toString.call(arg) === "[object RegExp]";
function readAllFile(root, reg) {
  let resultArr = [];
  try {
    if (fs__default.existsSync(root)) {
      const stat = fs__default.lstatSync(root);
      if (stat.isDirectory()) {
        const files = fs__default.readdirSync(root);
        files.forEach(function(file) {
          const t = readAllFile(path__default.join(root, "/", file), reg);
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

const debug = Debug__default.debug("vite-plugin-compression");
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
      outputPath = path__default.isAbsolute(config.build.outDir) ? config.build.outDir : path__default.join(config.root, config.build.outDir);
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
        const { mtimeMs, size: oldSize } = await fs__default$1.stat(filePath);
        if (mtimeMs <= (mtimeCache.get(filePath) || 0) || oldSize < threshold)
          return;
        let content = await fs__default$1.readFile(filePath);
        if (deleteOriginFile) {
          fs__default$1.remove(filePath);
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
        await fs__default$1.writeFile(cname, content);
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
      level: zlib__default.constants.Z_BEST_COMPRESSION
    },
    deflate: {
      level: zlib__default.constants.Z_BEST_COMPRESSION
    },
    deflateRaw: {
      level: zlib__default.constants.Z_BEST_COMPRESSION
    },
    brotliCompress: {
      params: {
        [zlib__default.constants.BROTLI_PARAM_QUALITY]: zlib__default.constants.BROTLI_MAX_QUALITY,
        [zlib__default.constants.BROTLI_PARAM_MODE]: zlib__default.constants.BROTLI_MODE_TEXT
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
    zlib__default[algorithm](content, options, (err, result) => err ? reject(err) : resolve(result));
  });
}
function getOutputFileName(filepath, ext) {
  const compressExt = ext.startsWith(".") ? ext : `.${ext}`;
  return `${filepath}${compressExt}`;
}
function handleOutputLogger(config, compressMap, algorithm) {
  config.logger.info(`
${chalk__default.cyan("\u2728 [vite-plugin-compression]:algorithm=" + algorithm)} - compressed file successfully: `);
  const keyLengths = Array.from(compressMap.keys(), (name) => name.length);
  const maxKeyLength = Math.max(...keyLengths);
  compressMap.forEach((value, name) => {
    const { size, oldSize, cname } = value;
    const rName = vite.normalizePath(cname).replace(vite.normalizePath(`${config.build.outDir}/`), "");
    const sizeStr = `${oldSize.toFixed(2)}kb / ${algorithm}: ${size.toFixed(2)}kb`;
    config.logger.info(chalk__default.dim(path__default.basename(config.build.outDir) + "/") + chalk__default.blueBright(rName) + " ".repeat(2 + maxKeyLength - name.length) + " " + chalk__default.dim(sizeStr));
  });
  config.logger.info("\n");
}

module.exports = index;

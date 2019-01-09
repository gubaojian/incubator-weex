const require = name => {
  const ret = {};
  const weexModuleName = name.replace(/^@weex-module\//, "");
  if (!weexModuleName) return ret;
  const nativeModule = __requireModule(weexModuleName);
  if (!nativeModule) return ret;
  const moduleMethodList = nativeModule[weexModuleName];
  if (!Array.isArray(moduleMethodList) || !moduleMethodList.length) return ret;
  moduleMethodList.forEach(methodName => {
    ret[methodName] = function() {
      return __callNativeModule({
        module: weexModuleName,
        method: methodName,
        args: arguments
      });
    };
  });
  return ret;
};

let document = {
  anchors: [],
  body: {},
  documentElement: {},
  documentURI: __weex_options__.bundleUrl,
  title: "",
  URL: __weex_options__.bundleUrl,
  visibilityState: "visible"
};

document.__eventMap = {};

document.documentElement.addEvent = ((type, handler) => {
  const eventMap = document.__eventMap || {};
  eventMap[type] = eventMap[type] || [];
  "function" === typeof handler && eventMap[type].push(handler);
});

__registerDocumentEventHandler((eventType, dataStr) => {
  let data = {};
  try {
    data = JSON.parse(dataStr);
  } catch (e) {}
  const eventObject = {
    type: eventType,
    data: data
  };
  const eventMap = document.__eventMap || {};
  const handlerList = eventMap[eventType] || [];
  handlerList.forEach(handler => {
    "function" == typeof handler && handler(eventObject);
  });
});

let open = url => {
  const weexNavigator = require("@weex-module/navigator");
  weexNavigator.push({
    url: url,
    animated: "true"
  }, function() {});
};

let console = {
  debug: function() {
    return __print(...arguments);
  },
  log: function() {
    return __print(...arguments);
  },
  info: function() {
    return __print(...arguments);
  },
  warn: function() {
    return __print(...arguments);
  },
  error: function() {
    return __print(...arguments);
  }
};

[ "dir", "dirxml", "table", "trace", "group", "groupCollapsed", "groupEnd", "clear", "count", "countReset", "assert", "profile", "profileEnd", "time", "timeLog", "timeEnd", "timeStamp", "context", "memory" ].forEach(method => {
  console[method] = function() {};
});

const location = {};

let screen = {
  width: WXEnvironment.deviceWidth,
  height: WXEnvironment.deviceHeight,
  vailWidth: WXEnvironment.deviceWidth,
  availHeight: WXEnvironment.deviceHeight,
  colorDepth: 24,
  pixelDepth: 24
};

const window = {
  console: console,
  document: document,
  location: location,
  open: open,
  screen: screen
};

let weex = {
  config: {
    bundleType: "Rax",
    bundleUrl: __weex_options__.bundleUrl,
    env: WXEnvironment
  },
  document: document,
  requireModule: require,
  importScript: () => {},
  isRegisteredModule: __isRegisteredModule,
  isRegisteredComponent: __isRegisteredComponent
};

const isWeex = true;

const isWeb = false;

const isNode = false;

const isReactNative = false;

class Component {
  constructor(props) {
    this.props = props;
  }
  isComponentClass() {}
  setState(updater, callback) {
    __setState(this, updater);
  }
}

const setNativeProps = () => {};

const findDOMNode = () => {};

const createElement = __createElement;

const render = __render;

let __weex_document__ = document, __weex_config__ = __weex_options__, __weex_module_supports__ = weex.isRegisteredModule, __weex_tag_supports__ = weex.isRegisteredComponent, __weex_define__ = () => {}, __weex_require__ = require, __weex_downgrade__ = () => {}, __weex_env__ = WXEnvironment, __weex_code__ = "";

!function(modules) {
  var installedModules = {};
  function __webpack_require__(moduleId) {
    if (installedModules[moduleId]) return installedModules[moduleId].exports;
    var module = installedModules[moduleId] = {
      i: moduleId,
      l: false,
      exports: {}
    };
    modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
    module.l = true;
    return module.exports;
  }
  __webpack_require__.m = modules;
  __webpack_require__.c = installedModules;
  __webpack_require__.d = function(exports, name, getter) {
    __webpack_require__.o(exports, name) || Object.defineProperty(exports, name, {
      enumerable: true,
      get: getter
    });
  };
  __webpack_require__.r = function(exports) {
    "undefined" !== typeof Symbol && Symbol.toStringTag && Object.defineProperty(exports, Symbol.toStringTag, {
      value: "Module"
    });
    Object.defineProperty(exports, "__esModule", {
      value: true
    });
  };
  __webpack_require__.t = function(value, mode) {
    1 & mode && (value = __webpack_require__(value));
    if (8 & mode) return value;
    if (4 & mode && "object" === typeof value && value && value.__esModule) return value;
    var ns = Object.create(null);
    __webpack_require__.r(ns);
    Object.defineProperty(ns, "default", {
      enumerable: true,
      value: value
    });
    if (2 & mode && "string" != typeof value) for (var key in value) __webpack_require__.d(ns, key, function(key) {
      return value[key];
    }.bind(null, key));
    return ns;
  };
  __webpack_require__.n = function(module) {
    var getter = module && module.__esModule ? function getDefault() {
      return module.default;
    } : function getModuleExports() {
      return module;
    };
    __webpack_require__.d(getter, "a", getter);
    return getter;
  };
  __webpack_require__.o = function(object, property) {
    return Object.prototype.hasOwnProperty.call(object, property);
  };
  __webpack_require__.p = "";
  __webpack_require__(__webpack_require__.s = 10);
}([ function(module, exports) {
  module.exports = require("@weex-module/userTrack");
}, function(module, exports, __webpack_require__) {
  module.exports = __webpack_require__(7);
}, function(module, exports, __webpack_require__) {
  module.exports = __webpack_require__(11);
}, function(module, exports) {
  module.exports = require("@weex-module/xsearchEvent");
}, function(module, exports) {
  if ("undefined" === typeof require("@weex-module/keyboard")) {
    var e = new Error("Cannot find module 'require(\"@weex-module/keyboard\")'");
    e.code = "MODULE_NOT_FOUND";
    throw e;
  }
  module.exports = require("@weex-module/keyboard");
}, function(module, exports, __webpack_require__) {
  "use strict";
  module.exports = __webpack_require__(8);
}, function(module, exports) {
  module.exports = require("@weex-module/windvane");
}, function(module, __webpack_exports__, __webpack_require__) {
  "use strict";
  __webpack_require__.r(__webpack_exports__);
  let GLOBAL_SPM;
  isNode && (global.window = {});
  if ("undefined" !== typeof window.__UNIVERSAL_SPM__) GLOBAL_SPM = window.__UNIVERSAL_SPM__; else {
    let spmAB = [ "0", "0" ];
    GLOBAL_SPM = {
      getPageSPM() {
        if (isWeb && window.goldlog) {
          const spm_ab = window.goldlog.spm_ab;
          spmAB = spm_ab && Array.isArray(spm_ab) && "0.0" !== spm_ab.join(".") ? spm_ab : spmAB;
        }
        return [ spmAB[0], spmAB[1] ];
      },
      getSPM(c, d) {
        return [].concat(this.getPageSPM(), c || 0, d || 0);
      },
      getSPMQueryString(c, d) {
        return "spm=" + this.getSPM(c, d).join(".");
      },
      setPageSPM(a, b) {
        spmAB[0] = a;
        spmAB[1] = b;
        if (isWeex) ; else if (isWeb) if (window.goldlog && window.goldlog.setPageSPM) window.goldlog.setPageSPM(a, b); else {
          const q = window.goldlog_queue || (window.goldlog_queue = []);
          q.push({
            action: "goldlog.setPageSPM",
            arguments: [ a, b ]
          });
        }
      }
    };
    window.__UNIVERSAL_SPM__ = GLOBAL_SPM;
  }
  __webpack_exports__.default = GLOBAL_SPM;
}, function(module, exports, __webpack_require__) {
  module.exports = __webpack_require__(12);
}, function(module, exports) {
  module.exports = require("@weex-module/prefetch");
}, function(module, __webpack_exports__, __webpack_require__) {
  "use strict";
  __webpack_require__.r(__webpack_exports__);
  const View = props => createElement("div", props);
  var rax_view = View;
  const Text = props => {
    props && props.numberOfLines && (props.style ? props.style.lines = props.numberOfLines : props.style = {
      lines: props.numberOfLines
    });
    return createElement("text", props);
  };
  var rax_text = Text;
  function _objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        _defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function _defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
  const styles = {
    container: {
      height: 30,
      justifyContent: "center",
      alignItems: "center",
      backgroundImage: "linear-gradient(to left, #1680FF, #2DA6FC)",
      borderRadius: 6
    },
    text: {
      fontSize: 20,
      letterSpcing: 0,
      color: "#fff"
    }
  };
  const Icon = _ref => {
    let text = _ref.text, width = _ref.width, style = _ref.style;
    return createElement(rax_view, {
      style: _objectSpread({}, styles.container, {
        width: width
      }, style)
    }, createElement(rax_text, {
      style: styles.text
    }, text));
  };
  var lib_Icon = Icon;
  function _extends() {
    _extends = Object.assign || function(target) {
      for (var i = 1; i < arguments.length; i++) {
        var source = arguments[i];
        for (var key in source) Object.prototype.hasOwnProperty.call(source, key) && (target[key] = source[key]);
      }
      return target;
    };
    return _extends.apply(this, arguments);
  }
  const NXImage = props => createElement("image", _extends({
    resizeMode: "cover"
  }, props));
  var nx_image = NXImage;
  function Link_extends() {
    Link_extends = Object.assign || function(target) {
      for (var i = 1; i < arguments.length; i++) {
        var source = arguments[i];
        for (var key in source) Object.prototype.hasOwnProperty.call(source, key) && (target[key] = source[key]);
      }
      return target;
    };
    return Link_extends.apply(this, arguments);
  }
  function _objectWithoutProperties(source, excluded) {
    if (null == source) return {};
    var target = _objectWithoutPropertiesLoose(source, excluded);
    var key, i;
    if (Object.getOwnPropertySymbols) {
      var sourceSymbolKeys = Object.getOwnPropertySymbols(source);
      for (i = 0; i < sourceSymbolKeys.length; i++) {
        key = sourceSymbolKeys[i];
        excluded.indexOf(key) >= 0 || Object.prototype.propertyIsEnumerable.call(source, key) && (target[key] = source[key]);
      }
    }
    return target;
  }
  function _objectWithoutPropertiesLoose(source, excluded) {
    if (null == source) return {};
    var target = {};
    var sourceKeys = Object.keys(source);
    var key, i;
    for (i = 0; i < sourceKeys.length; i++) {
      key = sourceKeys[i];
      excluded.indexOf(key) >= 0 || (target[key] = source[key]);
    }
    return target;
  }
  const safeEval = f => {
    try {
      "function" === typeof f && f();
    } catch (error) {}
  };
  class Link_Link extends Component {
    constructor() {
      super(...arguments);
      this.handleClick = (() => {
        const _this$props = this.props, onPress = _this$props.onPress, onClick = _this$props.onClick, href = _this$props.href;
        safeEval(onPress);
        safeEval(onClick);
        href && window.open(href);
      });
    }
    render() {
      const _this$props2 = this.props, extra = (_this$props2.href, _this$props2.onPress, 
      _this$props2.onClick, _objectWithoutProperties(_this$props2, [ "href", "onPress", "onClick" ]));
      return createElement(rax_view, Link_extends({
        onClick: this.handleClick
      }, extra));
    }
  }
  var lib_Link = Link_Link;
  const arrowImg = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAsAAAASBAMAAAB/WzlGAAAABGdBTUEAALGPC/xhBQAAAAFzUkdCAK7OHOkAAAAeUExURUdwTJmZmaKiopqampqampqampmZmZqampmZmZqamkl+ingAAAAKdFJOUwD+CntkucapllQSG9AUAAAAO0lEQVQI12PIYAADwwYwxSwB5RZAuOIKYNoxCEyxiEK5SWCKUwyZmggWZBFTQGiAqjcMgpiFbHIEmAQA16MHTKoWUcQAAAAASUVORK5CYII=";
  const subArrow = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAASBAMAAACgFUNZAAAABGdBTUEAALGPC/xhBQAAAAFzUkdCAK7OHOkAAAAkUExURUdwTJmZmaKiopqampqampqampqampqampqampmZmZqampmZmWQfaRkAAAAMdFJOUwD+CnpkxY+wvKNUzk2/WMYAAABESURBVAjXY2Bg4E5ggAA2MSiDwXAClMEsCRcqgjJYxBWgLMcmmJAoTGhhABqDCyblGARVK6GAqpsZZp5hAZqdHGCHAQCXJAhRzYGHCAAAAABJRU5ErkJggg==";
  const tbExIcon = "https://gw.alicdn.com/tfs/TB1IpciaQvoK1RjSZFDXXXY3pXa-80-80.png";
  const transferString = content => {
    var string = content;
    try {
      string = string.replace(/\r\n/g, " ");
      string = string.replace(/\n/g, " ");
      string = string.replace(/\r/g, " ");
      string = string.replace(/\t/g, " ");
    } catch (e) {
      string = content;
    }
    return string;
  };
  const pushTo = function(url) {
    !(arguments.length > 1 && void 0 !== arguments[1]) || arguments[1];
    url && window.open(url);
  };
  const mode = "m";
  const picPageBaseUrl = "https://market." + mode + ".taobao.com/app/tbsearchwireless-pages/experience-image-view/p/experience-image-view?wh_weex=true&wx_navbar_hidden=true&_wx_tpl=https://market." + mode + ".taobao.com/app/tbsearchwireless-pages/experience-image-view/p/experience-image-view";
  const getPicPageUrl = i => picPageBaseUrl + "&imageViewStorageKey=imageViewStorageKey&index=" + i;
  const poplayerPush = url => {
    const windvane = __webpack_require__(6);
    windvane.call2("TBXSearchPoplayerBridge.start", {
      url: url
    });
  };
  const getGoodsUrl = id => "https://item.taobao.com/item.htm?id=" + id + "&spm=a2141.tiezi.0.0";
  const getStorageData = props => {
    const subTitle = props.subTitle, topLinkUrl = props.topLinkUrl, imgs = props.imgs, items = props.items, rn = props.rn, keyword = props.keyword, trace = props.trace, contentId = props.contentId, pos = props.pos;
    const goods = imgs.map((_ref, i) => {
      let img = _ref.img;
      return {
        img: img,
        smallImg: img,
        price: "\u67e5\u770b\u8be6\u60c5",
        id: Array.isArray(items) ? items[i] : null,
        url: getGoodsUrl(items[i])
      };
    });
    const data = {
      articleText: subTitle,
      articleUrl: topLinkUrl,
      goods: goods,
      rn: rn,
      keyword: keyword,
      trace: trace,
      contentId: contentId,
      pos: pos
    };
    return data;
  };
  let storage_storage = {};
  isWeex ? storage_storage = __weex_require__("@weex-module/storage") : "undefined" !== typeof localStorage && localStorage && (storage_storage = localStorage);
  const AsyncStorage = {
    getItem: key => new Promise(function(resolve, reject) {
      if (storage_storage.getItem) if (isWeex) storage_storage.getItem(key, function(_ref) {
        let data = _ref.data, result = _ref.result;
        "success" === result ? resolve(void 0 === data ? null : data) : reject(data);
      }); else {
        let value = storage_storage.getItem(key);
        resolve(value);
      }
    }),
    setItem: (key, value) => new Promise(function(resolve, reject) {
      if (storage_storage.setItem) if (isWeex) storage_storage.setItem(key, value, function(_ref2) {
        let data = _ref2.data, result = _ref2.result;
        "success" === result ? resolve(data) : reject(data);
      }); else {
        storage_storage.setItem(key, value);
        resolve(null);
      }
    }),
    removeItem: key => new Promise(function(resolve, reject) {
      if (storage_storage.removeItem) if (isWeex) storage_storage.removeItem(key, function(_ref3) {
        let data = _ref3.data, result = _ref3.result;
        "success" === result ? resolve(data) : reject(data);
      }); else {
        storage_storage.removeItem(key);
        resolve(null);
      }
    }),
    getAllKeys: () => new Promise(function(resolve, reject) {
      isWeex ? storage_storage.getAllKeys(function(_ref4) {
        let data = _ref4.data, result = _ref4.result;
        "success" === result ? resolve(data) : reject(data);
      }) : storage_storage.length && resolve(Object.keys(storage_storage));
    }),
    clear: () => new Promise(function(resolve, reject) {
      if (storage_storage.clear) {
        storage_storage.clear();
        resolve(null);
      } else AsyncStorage.getAllKeys().then(function(keys) {
        Promise.all(keys.map(key => AsyncStorage.removeItem(key))).then(() => resolve(null)).catch(reject);
      });
    }),
    length: () => new Promise(function(resolve, reject) {
      isWeex ? storage_storage.length(function(_ref5) {
        let data = _ref5.data, result = _ref5.result;
        "success" === result ? resolve(data) : reject(data);
      }) : null != storage_storage.length && resolve(storage_storage.length);
    })
  };
  var lib_storage = AsyncStorage;
  var universal_goldlog = __webpack_require__(2);
  var universal_goldlog_default = __webpack_require__.n(universal_goldlog);
  const spmA = "a2141";
  const topSpmB = "topdetail";
  const utArg1 = "ConentDetail";
  const expArg1 = "ConentExposure";
  const topBussinessSpm = spmA + "." + topSpmB;
  const source = "taomiji";
  const bottomExtraClk = () => {
    universal_goldlog_default.a.record("/hotspot.daren.bottomdaren", "CLK", "", "GET");
  };
  const topExtraClk = () => {
    universal_goldlog_default.a.record("/hotspot.daren.toptitle", "CLK", "", "GET");
  };
  const middleExtraClk = () => {
    universal_goldlog_default.a.record("/hotspot.daren.middlepic", "CLK", "", "GET");
  };
  function ut_objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        ut_defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function ut_defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
  function processListParam() {
    let trace = arguments.length > 0 && void 0 !== arguments[0] ? arguments[0] : {};
    trace && trace.utLogMap && trace.utLogMap.list_param && (trace.utLogMap.list_param = encodeURIComponent(trace.utLogMap.list_param));
    return trace;
  }
  function auctionFormatedUrl(url, trace) {
    let status = arguments.length > 2 && void 0 !== arguments[2] ? arguments[2] : {};
    const urlSegments = url.split("?");
    const param = {};
    Array.isArray(urlSegments) && 2 === urlSegments.length && urlSegments[1].split("&").forEach(query => {
      const querySegments = query.split("=");
      2 === querySegments.length && (param[querySegments[0]] = querySegments[1]);
    });
    const _status$rn = status.rn, rn = void 0 === _status$rn ? "" : _status$rn, _status$abtest = status.abtest, abtest = void 0 === _status$abtest ? "" : _status$abtest, _status$keyword = status.keyword, keyword = void 0 === _status$keyword ? "" : _status$keyword;
    const _status$listType = status.listType, listType = void 0 === _status$listType ? "search" : _status$listType, _status$listParam = status.listParam, listParam = void 0 === _status$listParam ? keyword + "_" + abtest + "_" + rn : _status$listParam, index = status.index;
    const _trace$utLogMap = trace.utLogMap, utLogMap = void 0 === _trace$utLogMap ? {} : _trace$utLogMap;
    const newUtLogMap = ut_objectSpread({
      index: index
    }, utLogMap);
    const newQueryMap = ut_objectSpread({
      utparam: JSON.stringify(newUtLogMap),
      from: "search",
      list_type: listType,
      list_param: listParam
    }, param);
    let processedUrl = urlSegments[0] + "?";
    let and = "";
    Object.keys(newQueryMap).forEach(key => {
      processedUrl += and + (key + "=") + encodeURIComponent(newQueryMap[key]);
      and = "&";
    });
    return processedUrl;
  }
  function auctionClick(controlName) {
    let trace = arguments.length > 1 && void 0 !== arguments[1] ? arguments[1] : {};
    let status = arguments.length > 2 && void 0 !== arguments[2] ? arguments[2] : {};
    const _status$rn2 = status.rn, rn = void 0 === _status$rn2 ? "" : _status$rn2, _status$abtest2 = status.abtest, abtest = void 0 === _status$abtest2 ? "" : _status$abtest2, _status$keyword2 = status.keyword, keyword = void 0 === _status$keyword2 ? "" : _status$keyword2;
    const _status$listType2 = status.listType, listType = void 0 === _status$listType2 ? "search" : _status$listType2, _status$listParam2 = status.listParam, listParam = void 0 === _status$listParam2 ? encodeURIComponent(keyword) + "_" + abtest + "_" + rn : _status$listParam2, index = status.index;
    const _status$hubbleInfo = status.hubbleInfo, hubbleInfo = void 0 === _status$hubbleInfo ? {} : _status$hubbleInfo, pageName = status.pageName;
    let newTrace = {};
    try {
      newTrace = processListParam(JSON.parse(JSON.stringify(trace)));
    } catch (error) {}
    Object.keys(newTrace).forEach(key => {
      "object" === typeof newTrace[key] && (newTrace[key] = encodeURIComponent(JSON.stringify(newTrace[key])));
    });
    const args = ut_objectSpread({}, newTrace, {
      index: index,
      from: "search",
      list_type: listType,
      list_param: listParam,
      hubbleTemplateInfo: encodeURIComponent(JSON.stringify(hubbleInfo))
    });
    pageName && (args.pageName = pageName);
    const userTrack = __webpack_require__(0);
    userTrack.commitut("click", -1, pageName, "Button-" + controlName, pageName + "_Button-" + controlName, "", "", args);
  }
  function exposure_objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        exposure_defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function exposure_defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
  function exposure_processListParam() {
    let trace = arguments.length > 0 && void 0 !== arguments[0] ? arguments[0] : {};
    trace && trace.utLogMap && trace.utLogMap.list_param && (trace.utLogMap.list_param = encodeURIComponent(trace.utLogMap.list_param));
    return trace;
  }
  function expose(controlName) {
    let trace = arguments.length > 1 && void 0 !== arguments[1] ? arguments[1] : {};
    let state = arguments.length > 2 && void 0 !== arguments[2] ? arguments[2] : {};
    let extraParams = arguments.length > 3 && void 0 !== arguments[3] ? arguments[3] : {};
    let newTrace = {};
    try {
      newTrace = exposure_processListParam(JSON.parse(JSON.stringify(trace)));
    } catch (error) {}
    Object.keys(newTrace).forEach(key => {
      "object" === typeof newTrace[key] && (newTrace[key] = encodeURIComponent(JSON.stringify(newTrace[key])));
    });
    const _state$data = state.data, _state$data$status = _state$data.status, status = void 0 === _state$data$status ? {} : _state$data$status, _state$data$storage = _state$data.storage, storage = void 0 === _state$data$storage ? {} : _state$data$storage, _state$data$status$pa = _state$data.status.pageName, pageName = void 0 === _state$data$status$pa ? "" : _state$data$status$pa;
    const _status$hubbleInfo = status.hubbleInfo, hubbleInfo = void 0 === _status$hubbleInfo ? {} : _status$hubbleInfo;
    let _storage$cellExposed = storage.cellExposed, cellExposed = void 0 === _storage$cellExposed ? "false" : _storage$cellExposed;
    if ("true" === cellExposed) return;
    let newStorage = exposure_objectSpread({}, storage, {
      cellExposed: "true"
    });
    const xsearchEvent = __webpack_require__(3);
    const userTrack = __webpack_require__(0);
    if (xsearchEvent && "function" === typeof xsearchEvent.updateStorage) {
      xsearchEvent.updateStorage(newStorage);
      userTrack.commitut("expose", -1, pageName, "Button-" + controlName, pageName + "_Button-" + controlName, "", "", exposure_objectSpread({}, newTrace, extraParams, {
        hubbleTemplateInfo: encodeURIComponent(JSON.stringify(hubbleInfo))
      }));
    }
  }
  function auctionExpose(controlName) {
    let trace = arguments.length > 1 && void 0 !== arguments[1] ? arguments[1] : {};
    let state = arguments.length > 2 && void 0 !== arguments[2] ? arguments[2] : {};
    const _state$data2 = state.data, _state$data2$status = _state$data2.status, status = void 0 === _state$data2$status ? {} : _state$data2$status, _state$data2$status2 = _state$data2.status, _state$data2$status2$ = _state$data2$status2.rn, rn = void 0 === _state$data2$status2$ ? "" : _state$data2$status2$, _state$data2$status2$2 = _state$data2$status2.abtest, abtest = void 0 === _state$data2$status2$2 ? "" : _state$data2$status2$2, _state$data2$status2$3 = _state$data2$status2.keyword, keyword = void 0 === _state$data2$status2$3 ? "" : _state$data2$status2$3;
    const _status$listType = status.listType, listType = void 0 === _status$listType ? "search" : _status$listType, _status$listParam = status.listParam, listParam = void 0 === _status$listParam ? encodeURIComponent(keyword) + "_" + abtest + "_" + rn : _status$listParam, index = status.index;
    expose(controlName, trace, state, {
      listType: listType,
      listParam: listParam,
      index: index
    });
  }
  function WidgetInListStyle_objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        WidgetInListStyle_defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function WidgetInListStyle_defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
  class WidgetInListStyle_WidgetInListStyle extends Component {
    constructor() {
      super(...arguments);
      this.goldlogClick = (() => {
        const onClick = this.props.onClick;
        "function" === typeof onClick && onClick();
      });
      this.onTopClick = (() => {
        this.goldlogClick();
        topExtraClk();
      });
      this.onBottomClick = (() => {
        this.goldlogClick();
        bottomExtraClk();
        const _this$props = this.props, toTbex = _this$props.toTbex, _this$props$bottomUrl = _this$props.bottomUrl, bottomUrl = void 0 === _this$props$bottomUrl ? "" : _this$props$bottomUrl;
        const isToTbEx = true === toTbex || "true" === toTbex;
        if (isWeex) {
          const xsearchEvent = __webpack_require__(3);
          xsearchEvent && "function" === typeof xsearchEvent.jumpToTab && isToTbEx ? xsearchEvent.jumpToTab({
            tab: "tbexperience"
          }) : pushTo(bottomUrl);
        } else pushTo(bottomUrl);
      });
      this.onSubTitleClick = (() => {
        this.goldlogClick();
        topExtraClk();
        const _this$props$topLinkUr = this.props.topLinkUrl, topLinkUrl = void 0 === _this$props$topLinkUr ? "" : _this$props$topLinkUr;
        pushTo(topLinkUrl);
      });
      this.onClickFactory = (index => () => {
        const _this$props2 = this.props, trace = _this$props2.trace, status = _this$props2.status;
        this.goldlogClick();
        middleExtraClk();
        const data = getStorageData(this.props);
        lib_storage.setItem("imageViewStorageKey", JSON.stringify(data)).then(() => {
          poplayerPush(auctionFormatedUrl(getPicPageUrl(index), trace, status), false);
        });
      });
    }
    render() {
      try {
        return this.renderComponent();
      } catch (e) {
        return createElement(rax_view, {
          style: {
            height: 1,
            width: 750
          }
        });
      }
    }
    renderComponent() {
      const _this$props3 = this.props, _this$props3$icon = _this$props3.icon, icon = void 0 === _this$props3$icon ? "" : _this$props3$icon, _this$props3$title = _this$props3.title, title = void 0 === _this$props3$title ? "" : _this$props3$title, _this$props3$subTitle = _this$props3.subTitle, subTitle = void 0 === _this$props3$subTitle ? "" : _this$props3$subTitle, _this$props3$imgs = _this$props3.imgs, imgs = void 0 === _this$props3$imgs ? [] : _this$props3$imgs, _this$props3$topLinkU = _this$props3.topLinkUrl, topLinkUrl = void 0 === _this$props3$topLinkU ? "" : _this$props3$topLinkU, _this$props3$bottomIc = _this$props3.bottomIcon, bottomIcon = void 0 === _this$props3$bottomIc ? "" : _this$props3$bottomIc, _this$props3$bottomTe = _this$props3.bottomText, bottomText = void 0 === _this$props3$bottomTe ? "" : _this$props3$bottomTe, toTbex = _this$props3.toTbex, dacuIcon = _this$props3.dacuIcon;
      const isShow = title && subTitle && Array.isArray(imgs) && imgs.length >= 3;
      if (!isShow) return createElement(rax_view, {
        style: {
          height: 1,
          width: 750
        }
      });
      let width, titleStyle, iconElement;
      if (dacuIcon) {
        const dacuIconUrl = dacuIcon.dacuIconUrl, dacuIconHeight = dacuIcon.dacuIconHeight, dacuIconWidth = dacuIcon.dacuIconWidth;
        if (dacuIconUrl && dacuIconHeight && dacuIconWidth) {
          width = 30 * dacuIconWidth / dacuIconHeight;
          iconElement = createElement(nx_image, {
            style: WidgetInListStyle_objectSpread({}, WidgetInListStyle_styles.icon, {
              width: width,
              height: 30
            }),
            src: dacuIconUrl
          });
          titleStyle = WidgetInListStyle_objectSpread({}, WidgetInListStyle_styles.title, {
            width: 692 - width
          });
        } else {
          titleStyle = WidgetInListStyle_objectSpread({}, WidgetInListStyle_styles.title, {
            width: 692
          });
          iconElement = null;
        }
      } else if (icon) {
        const len = String(icon).length;
        width = 21 * len + 12;
        titleStyle = WidgetInListStyle_objectSpread({}, WidgetInListStyle_styles.title, {
          width: 692 - width
        });
        iconElement = createElement(lib_Icon, {
          text: icon,
          width: width,
          style: WidgetInListStyle_styles.icon
        });
      } else {
        titleStyle = WidgetInListStyle_objectSpread({}, WidgetInListStyle_styles.title, {
          width: 692
        });
        iconElement = null;
      }
      const line1TextNum = 2 * Math.ceil(702 / 29) - 8;
      const subTitleText = subTitle.slice(0, line1TextNum) + "... ";
      const isToTbEx = true === toTbex || "true" === toTbex;
      const bottomTextFinal = isToTbEx ? "\u6dd8\u5b9d\u7ecf\u9a8c" : bottomText;
      const bottomIconFinal = isToTbEx ? tbExIcon : bottomIcon;
      return createElement(rax_view, {
        style: WidgetInListStyle_styles.container
      }, title ? createElement(rax_view, {
        style: WidgetInListStyle_styles.titleContainer
      }, iconElement, createElement(rax_text, {
        numberOfLines: 1,
        style: titleStyle
      }, title)) : null, createElement(rax_view, {
        style: WidgetInListStyle_styles.subTitleContainer,
        key: "View" + Date.now()
      }, createElement("p", {
        key: "p" + Date.now(),
        style: WidgetInListStyle_styles.subTitleLine1
      }, createElement("span", null, subTitleText), createElement("a", {
        style: WidgetInListStyle_objectSpread({}, goStyles.container)
      }, createElement("span", {
        style: goStyles.text
      }, "\u5168\u6587"), createElement("img", {
        alt: "",
        src: subArrow,
        style: goStyles.image
      })))), createElement(rax_view, {
        style: {
          height: 22,
          backgroundColor: "#fff",
          width: 702
        }
      }), createElement(rax_view, {
        style: WidgetInListStyle_styles.imageContainer
      }, Array.isArray(imgs) ? imgs.slice(0, 3).map((_ref, index) => {
        let img = _ref.img;
        return createElement(rax_view, {
          onClick: this.onClickFactory(index),
          style: WidgetInListStyle_styles.image
        }, createElement(nx_image, {
          resizeMode: "cover",
          style: WidgetInListStyle_styles.image,
          src: img,
          key: index
        }), createElement(rax_view, {
          style: WidgetInListStyle_styles.imageMask
        }));
      }) : null), createElement(lib_Link, {
        onPress: this.onTopClick,
        href: topLinkUrl,
        style: {
          width: 750,
          height: 156,
          position: "absolute",
          left: 0,
          top: 0
        }
      }), createElement(rax_view, {
        onClick: this.onBottomClick,
        style: WidgetInListStyle_styles.goto
      }, createElement(rax_view, {
        style: WidgetInListStyle_styles.bottomIconContainer
      }, bottomIconFinal ? createElement(nx_image, {
        style: WidgetInListStyle_styles.bottomIcon,
        src: bottomIconFinal
      }) : null, createElement(rax_text, {
        style: WidgetInListStyle_styles.bottomText
      }, bottomTextFinal), bottomIconFinal ? createElement(rax_view, {
        style: WidgetInListStyle_styles.bottomIconMask
      }) : null), createElement(nx_image, {
        style: WidgetInListStyle_styles.gotoImage,
        src: arrowImg
      })));
    }
  }
  const WidgetInListStyle_styles = {
    container: {
      width: 750,
      height: 496,
      paddingLeft: 24,
      paddingRight: 24,
      paddingTop: 24,
      paddingBottom: 24,
      backgroundColor: "#fff"
    },
    titleContainer: {
      height: 42,
      width: 702,
      flexDirection: "row",
      justifyContent: "flex-start",
      alignItems: "center",
      marginBottom: 6
    },
    icon: {
      marginRight: 10
    },
    title: {
      fontSize: 30,
      fontWeight: "bold",
      color: "#333333"
    },
    subTitleContainer: {
      width: 702,
      height: 84,
      marginTop: 0,
      marginBottom: 0,
      justifyContent: "center",
      lines: 2
    },
    subTitleLine1: {
      width: 702,
      lineHeight: 42,
      fontFamily: "PingFangSC-Light",
      fontSize: 28,
      color: "#666666",
      marginTop: 0,
      marginBottom: 0,
      lines: 2
    },
    subTitleLine2Container: {
      width: 702,
      height: 42,
      flexDirection: "row"
    },
    gotoFull: {
      position: "absolute",
      right: 0,
      bottom: 0
    },
    imageContainer: {
      width: 702,
      height: 230,
      marginBottom: 24,
      flexDirection: "row",
      justifyContent: "space-between"
    },
    image: {
      width: 230,
      height: 230,
      borderRadius: 6
    },
    imageMask: {
      width: 230,
      height: 230,
      borderRadius: 6,
      position: "absolute",
      top: 0,
      left: 0,
      backgroundColor: "rgba(0, 0, 0, 0.03)"
    },
    goto: {
      height: 40,
      width: 702,
      flexDirection: "row",
      alignItems: "center",
      justifyContent: "space-between"
    },
    bottomIconContainer: {
      flexDirection: "row",
      alignItems: "center"
    },
    bottomIcon: {
      width: 40,
      height: 40,
      marginRight: 10,
      borderRadius: 20
    },
    bottomIconMask: {
      width: 40,
      height: 40,
      borderRadius: 20,
      position: "absolute",
      top: 0,
      left: 0,
      backgroundColor: "rgba(0,0,0,0.02)"
    },
    bottomText: {
      fontSize: 24,
      color: "#999999"
    },
    gotoImage: {
      width: 10,
      height: 18
    }
  };
  const goStyles = {
    container: {
      width: 83,
      height: 42,
      flexDirection: "row",
      alignItems: "center"
    },
    text: {
      fontFamily: "PingFangSC-Light",
      fontSize: 28,
      color: "#9B9B9B"
    },
    image: {
      width: 16,
      height: 18,
      marginLeft: 5
    }
  };
  WidgetInListStyle_WidgetInListStyle.displayName = "WidgetInListStyle";
  var lib_WidgetInListStyle = WidgetInListStyle_WidgetInListStyle;
  function WidgetInWFStyle_objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        WidgetInWFStyle_defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function WidgetInWFStyle_defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
  class WidgetInWFStyle_WidgetInWFStyle extends Component {
    constructor() {
      super(...arguments);
      this.onClickHandler = (() => {
        const _this$props = this.props, trace = _this$props.trace, status = _this$props.status;
        this.goldlogClick();
        middleExtraClk();
        const index = 0;
        const data = getStorageData(this.props);
        lib_storage.setItem("imageViewStorageKey", JSON.stringify(data)).then(() => {
          poplayerPush(auctionFormatedUrl(getPicPageUrl(index), trace, status));
        });
      });
      this.goldlogClick = (() => {
        const onClick = this.props.onClick;
        "function" === typeof onClick && onClick();
      });
      this.onTopClick = (() => {
        this.goldlogClick();
        topExtraClk();
      });
      this.onBottomClick = (() => {
        this.goldlogClick();
        bottomExtraClk();
        const _this$props2 = this.props, toTbex = _this$props2.toTbex, _this$props2$bottomUr = _this$props2.bottomUrl, bottomUrl = void 0 === _this$props2$bottomUr ? "" : _this$props2$bottomUr;
        const isToTbEx = true === toTbex || "true" === toTbex;
        if (isWeex) {
          const xsearchEvent = __webpack_require__(3);
          xsearchEvent && "function" === typeof xsearchEvent.jumpToTab && isToTbEx ? xsearchEvent.jumpToTab({
            tab: "tbexperience"
          }) : pushTo(bottomUrl);
        } else pushTo(bottomUrl);
      });
    }
    render() {
      const _this$props3 = this.props, _this$props3$icon = _this$props3.icon, icon = void 0 === _this$props3$icon ? "" : _this$props3$icon, _this$props3$title = _this$props3.title, title = void 0 === _this$props3$title ? "" : _this$props3$title, _this$props3$subTitle = _this$props3.subTitle, subTitle = void 0 === _this$props3$subTitle ? "" : _this$props3$subTitle, _this$props3$imgs = _this$props3.imgs, imgs = void 0 === _this$props3$imgs ? [] : _this$props3$imgs, _this$props3$bottomIc = _this$props3.bottomIcon, bottomIcon = void 0 === _this$props3$bottomIc ? "" : _this$props3$bottomIc, _this$props3$bottomTe = _this$props3.bottomText, bottomText = void 0 === _this$props3$bottomTe ? "" : _this$props3$bottomTe, _this$props3$topLinkU = _this$props3.topLinkUrl, topLinkUrl = void 0 === _this$props3$topLinkU ? "" : _this$props3$topLinkU, toTbex = _this$props3.toTbex, dacuIcon = _this$props3.dacuIcon;
      const isShow = title && subTitle && Array.isArray(imgs) && imgs.length >= 3;
      if (!isShow) return createElement(rax_view, {
        style: {
          height: 1,
          width: 342
        }
      });
      const isToTbEx = true === toTbex || "true" === toTbex;
      const bottomTextFinal = isToTbEx ? "\u6dd8\u5b9d\u7ecf\u9a8c" : bottomText;
      const bottomIconFinal = isToTbEx ? tbExIcon : bottomIcon;
      let width, iconElement;
      if (dacuIcon) {
        const dacuIconUrl = dacuIcon.dacuIconUrl, dacuIconHeight = dacuIcon.dacuIconHeight, dacuIconWidth = dacuIcon.dacuIconWidth;
        width = 30 * dacuIconWidth / dacuIconHeight;
        iconElement = createElement(nx_image, {
          style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_styles.icon, {
            width: width,
            height: 30
          }),
          src: dacuIconUrl
        });
      } else if (icon) {
        const len = String(icon).length;
        width = len ? 21 * len + 12 : 0;
        iconElement = createElement(lib_Icon, {
          text: icon,
          width: width,
          style: WidgetInWFStyle_styles.icon
        });
      } else iconElement = null;
      return createElement(rax_view, {
        style: WidgetInWFStyle_styles.container
      }, createElement(rax_view, {
        style: WidgetInWFStyle_styles.imageContainer,
        onClick: this.onClickHandler
      }, createElement(rax_view, {
        style: WidgetInWFStyle_styles.img1
      }, createElement(nx_image, {
        style: WidgetInWFStyle_styles.img1,
        resizeMode: "cover",
        src: imgs[0].img
      }), createElement(rax_view, {
        style: WidgetInWFStyle_styles.mask
      })), createElement(rax_view, {
        style: WidgetInWFStyle_styles.img23Container
      }, createElement(rax_view, {
        style: WidgetInWFStyle_styles.img2
      }, createElement(nx_image, {
        style: WidgetInWFStyle_styles.img2,
        resizeMode: "cover",
        src: imgs[1].img
      }), createElement(rax_view, {
        style: WidgetInWFStyle_styles.mask
      })), createElement(rax_view, {
        style: WidgetInWFStyle_styles.img3
      }, createElement(nx_image, {
        style: WidgetInWFStyle_styles.img3,
        resizeMode: "cover",
        src: imgs[2].img
      }), createElement(rax_view, {
        style: WidgetInWFStyle_styles.mask
      })))), createElement(lib_Link, {
        href: topLinkUrl,
        onPress: this.onTopClick
      }, createElement(rax_text, {
        style: WidgetInWFStyle_styles.title,
        numberOfLines: 1
      }, title), createElement(rax_text, {
        style: WidgetInWFStyle_styles.subTitle,
        numberOfLines: 2
      }, subTitle)), createElement(rax_view, {
        style: WidgetInWFStyle_styles.splitLine
      }), createElement(rax_view, {
        onClick: this.onBottomClick,
        style: WidgetInWFStyle_styles.bottomContainer
      }, createElement(rax_view, {
        style: WidgetInWFStyle_styles.left
      }, createElement(nx_image, {
        style: WidgetInWFStyle_styles.leftIcon,
        src: bottomIconFinal
      }), createElement(rax_text, {
        style: WidgetInWFStyle_styles.leftText
      }, bottomTextFinal), createElement(rax_view, {
        style: WidgetInWFStyle_styles.leftIconMask
      })), createElement(nx_image, {
        style: WidgetInWFStyle_styles.rightIcon,
        src: arrowImg
      })), iconElement);
    }
  }
  const WidgetInWFStyle_styles = {
    container: {
      borderRadius: 10,
      width: 342,
      backgroundColor: "#fff",
      overflow: "hidden"
    },
    imageContainer: {
      width: 342,
      height: 342
    },
    img1: {
      width: 342,
      height: 168,
      marginBottom: 5
    },
    img23Container: {
      width: 342,
      height: 168,
      flexDirection: "row",
      justifyContent: "space-between"
    },
    img2: {
      width: 168,
      height: 168
    },
    img3: {
      width: 168,
      height: 168
    },
    mask: {
      position: "absolute",
      left: 0,
      right: 0,
      top: 0,
      bottom: 0,
      backgroundColor: "rgba(0, 0, 0, 0.03)"
    },
    title: {
      marginTop: 24,
      marginLeft: 24,
      marginRight: 31,
      marginBottom: 8,
      width: 287,
      fontSize: 26,
      lineHeight: 30,
      color: "#33333",
      fontWeight: "bold"
    },
    subTitle: {
      marginLeft: 24,
      marginRight: 31,
      marginBottom: 16,
      width: 287,
      fontSize: 26,
      lineHeight: 30,
      color: "#666666",
      textOverflow: "ellipsis"
    },
    splitLine: {
      backgroundColor: "#F1F1F1",
      height: "1px",
      width: 294,
      marginLeft: 24
    },
    bottomContainer: {
      marginLeft: 24,
      marginRight: 31,
      marginBottom: 25,
      marginTop: 16,
      width: 287,
      flexDirection: "row",
      alignItems: "center"
    },
    left: {
      flex: 1,
      flexDirection: "row",
      alignItems: "center"
    },
    leftIcon: {
      width: 40,
      height: 40,
      marginRight: 10,
      borderRadius: 20
    },
    leftIconMask: {
      width: 40,
      height: 40,
      marginRight: 10,
      position: "absolute",
      left: 0,
      top: 0,
      backgroundColor: "rgba(0,0,0,0.03)"
    },
    leftText: {
      fontSize: 24,
      color: "#999999"
    },
    rightIcon: {
      width: 10,
      height: 18
    },
    icon: {
      position: "absolute",
      left: 24,
      top: 327
    }
  };
  WidgetInWFStyle_WidgetInWFStyle.displayName = "WidgetInWFStyle";
  var lib_WidgetInWFStyle = WidgetInWFStyle_WidgetInWFStyle;
  var preFetchTask = () => {
    var jsprefetch = __webpack_require__(9);
    jsprefetch.addTask("https://market.m.taobao.com/app/tbsearchwireless-pages/experience-image-view/p/experience-image-view?wh_weex=true", []);
  };
  function lib_extends() {
    lib_extends = Object.assign || function(target) {
      for (var i = 1; i < arguments.length; i++) {
        var source = arguments[i];
        for (var key in source) Object.prototype.hasOwnProperty.call(source, key) && (target[key] = source[key]);
      }
      return target;
    };
    return lib_extends.apply(this, arguments);
  }
  try {
    preFetchTask();
  } catch (error) {}
  class lib_Widget extends Component {
    constructor(props) {
      super(props);
      this.expMap = {};
    }
    render() {
      const _this$props = this.props, _this$props$model = _this$props.model, model = void 0 === _this$props$model ? {} : _this$props$model, _this$props$status = _this$props.status, status = void 0 === _this$props$status ? {} : _this$props$status, _this$props$storage = _this$props.storage, storage = void 0 === _this$props$storage ? {} : _this$props$storage;
      const _status$layoutStyle = status.layoutStyle, layoutStyle = void 0 === _status$layoutStyle ? 0 : _status$layoutStyle;
      const content = 1 === parseInt(layoutStyle, 10) ? createElement(lib_WidgetInWFStyle, lib_extends({}, model, {
        status: status,
        storage: storage
      })) : createElement(lib_WidgetInListStyle, lib_extends({}, model, {
        status: status,
        storage: storage
      }));
      return content;
    }
  }
  var lib = lib_Widget;
  function dataParser_objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        dataParser_defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function dataParser_defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
  function dataParser_objectWithoutProperties(source, excluded) {
    if (null == source) return {};
    var target = dataParser_objectWithoutPropertiesLoose(source, excluded);
    var key, i;
    if (Object.getOwnPropertySymbols) {
      var sourceSymbolKeys = Object.getOwnPropertySymbols(source);
      for (i = 0; i < sourceSymbolKeys.length; i++) {
        key = sourceSymbolKeys[i];
        excluded.indexOf(key) >= 0 || Object.prototype.propertyIsEnumerable.call(source, key) && (target[key] = source[key]);
      }
    }
    return target;
  }
  function dataParser_objectWithoutPropertiesLoose(source, excluded) {
    if (null == source) return {};
    var target = {};
    var sourceKeys = Object.keys(source);
    var key, i;
    for (i = 0; i < sourceKeys.length; i++) {
      key = sourceKeys[i];
      excluded.indexOf(key) >= 0 || (target[key] = source[key]);
    }
    return target;
  }
  const addUrlParams = (url, extra) => {
    const hasQuery = String(url).indexOf("?") > 0;
    const extraQuery = Object.keys(extra).map(k => k + "=" + extra[k]).join("&");
    return extraQuery ? hasQuery ? url + "&" + extraQuery : url + "?" + extraQuery : url;
  };
  const urlFormat = function(trace, status) {
    let extra = arguments.length > 2 && void 0 !== arguments[2] ? arguments[2] : {};
    return url => {
      const urlWithExtra = addUrlParams(url, extra);
      return url ? auctionFormatedUrl(urlWithExtra, trace, status) : url;
    };
  };
  const safeMap = arr => f => Array.isArray(arr) ? arr.map(f) : [];
  const dataParser = data => {
    const model = data.model, _data$status = data.status, status = void 0 === _data$status ? {} : _data$status, ext = dataParser_objectWithoutProperties(data, [ "model", "status" ]);
    const _ref = model || {}, info = _ref.info, _ref$trace = _ref.trace, trace = void 0 === _ref$trace ? {} : _ref$trace;
    const rn = status.rn, keyword = status.keyword, index = status.index;
    const _ref2 = info || {}, mainTitle = _ref2.mainTitle, subTitle = _ref2.subTitle, topLinkUrl = _ref2.topLinkUrl, pics = _ref2.pics, bottomIcons = _ref2.bottomIcons, bottomText = _ref2.bottomText, bizName = _ref2.bizName, bottomLinkUrl = _ref2.bottomLinkUrl, contentId = _ref2.contentId, _ref2$toTbex = _ref2.toTbex, toTbex = void 0 === _ref2$toTbex ? "false" : _ref2$toTbex, _ref2$items = _ref2.items, items = void 0 === _ref2$items ? [] : _ref2$items, dacuPicIcon = _ref2.dacuPicIcon, dacuIcon = _ref2.dacuIcon, dacuIconHeight = _ref2.dacuIconHeight, dacuIconWidth = _ref2.dacuIconWidth;
    const topQuery = {
      business_spm: topBussinessSpm,
      source: source
    };
    const topUrl = urlFormat(trace, status, topQuery)(topLinkUrl);
    const bottomUrl = urlFormat(trace, status, {})(bottomLinkUrl);
    const onClick = () => {
      auctionClick(utArg1, trace, status);
    };
    const dacuIconFinal = "true" === dacuPicIcon || true === dacuPicIcon ? {
      dacuIconUrl: dacuIcon,
      dacuIconHeight: dacuIconHeight,
      dacuIconWidth: dacuIconWidth
    } : null;
    const parsedModel = {
      icon: bizName,
      title: transferString(mainTitle),
      subTitle: transferString(subTitle),
      topLinkUrl: topUrl,
      onClick: onClick,
      imgs: safeMap(pics)(img => ({
        img: img
      })),
      bottomUrl: bottomUrl,
      bottomIcon: Array.isArray(bottomIcons) ? bottomIcons[0] : null,
      bottomText: bottomText,
      trace: trace,
      contentId: contentId,
      toTbex: toTbex,
      items: items,
      dacuIcon: dacuIconFinal,
      status: status,
      rn: rn,
      keyword: keyword,
      pos: index
    };
    return dataParser_objectSpread({
      model: parsedModel,
      status: status,
      trace: trace
    }, ext);
  };
  const _data = __weex_data__;
  class src_Segment extends Component {
    constructor(props) {
      super(props);
      this.state = {
        data: _data
      };
    }
    componentWillMount() {
      setNativeProps(findDOMNode(document.body), {
        style: {
          backgroundColor: "transparent"
        }
      });
    }
    componentDidMount() {
      const self = this;
      document.documentElement.addEvent("refresh", evt => {
        let newData = evt.data;
        newData && self.setState({
          data: newData
        });
      });
    }
    render() {
      const _this$state$data = this.state.data, data = void 0 === _this$state$data ? {} : _this$state$data;
      const _data$model = data.model, model = void 0 === _data$model ? {} : _data$model;
      const trace = model.trace;
      isWeex && auctionExpose(expArg1, trace, this.state);
      const parsedData = dataParser(data);
      return createElement(lib, parsedData);
    }
  }
  render(createElement(src_Segment, null));
}, function(module, __webpack_exports__, __webpack_require__) {
  "use strict";
  __webpack_require__.r(__webpack_exports__);
  var universal_spm = __webpack_require__(1);
  var universal_spm_default = __webpack_require__.n(universal_spm);
  isNode && (global.window = {});
  function paramsToObj(str) {
    str = "string" === typeof str ? str : "";
    let result = {};
    let splitStr = str.split("&");
    for (let i = 0; i < splitStr.length; i++) {
      let s = splitStr[i];
      let splitKV = s.split("=");
      let key = splitKV[0];
      let val = splitKV[1];
      key && (result[key] = val);
    }
    return result;
  }
  function objToParams(obj, isEnCode) {
    let result = [];
    for (let i in obj) if (obj.hasOwnProperty(i)) {
      let key = i;
      let val = isEnCode ? encodeURIComponent(obj[i]) : obj[i];
      result.push(key + "=" + val);
    }
    return result.join("&");
  }
  function getParamFromURL(url) {
    let param = arguments.length > 1 && void 0 !== arguments[1] ? arguments[1] : "spm";
    let search = url.split("?")[1] || "";
    let paramValue = "";
    search.split("&").forEach(function(o) {
      0 === o.indexOf(param + "=") && (paramValue = o.substr(param.length + 1));
    });
    return paramValue;
  }
  function makeChkSum(s) {
    s = (s || "").split("#")[0].split("?")[0];
    const len = s.length;
    const hash = function(s) {
      const l = s.length;
      let key = 0;
      for (let i = 0; i < l; i++) key = 31 * key + s.charCodeAt(i);
      return key;
    };
    return len ? hash(len + "#" + s.charCodeAt(len - 1)) : -1;
  }
  function getMetaContentByName(metaName) {
    const meta = window && window.document && window.document.getElementsByTagName("meta")[metaName];
    return meta ? meta.getAttribute("content") : "";
  }
  function simplifyURL() {
    let url = arguments.length > 0 && void 0 !== arguments[0] ? arguments[0] : "";
    const WEEX_PREFIX = "_wx_tpl=";
    const WEEX_SUFFIX = ".js";
    url.indexOf(WEEX_PREFIX) > -1 && (url = url.substring(url.indexOf(WEEX_PREFIX) + WEEX_PREFIX.length, url.indexOf(WEEX_SUFFIX) + WEEX_SUFFIX.length));
    return url.split("?")[0];
  }
  let UserTrack;
  try {
    isWeex && (UserTrack = __webpack_require__(0));
  } catch (err) {
    UserTrack = {
      customAdvance: () => {},
      commit: () => {},
      commitut: () => {},
      enterEvent: () => {},
      updatePageUtparam: () => {},
      updateNextPageUtparam: () => {},
      commitut: () => {}
    };
  }
  const weex = "undefined" !== typeof __weex_options__ && __weex_options__.weex;
  const isInWindmill = isWeex && "object" === typeof weex && "windmill" === weex.config.container || false;
  let isThirdGroupAPI = false;
  if (isInWindmill) try {
    let keyboard = __webpack_require__(4);
    typeof keyboard && "function" == typeof keyboard.hideKeyboard && (isThirdGroupAPI = true);
  } catch (e) {}
  var userTrack = {
    customAdvance: (pageName, eventid, arg1, arg2, arg3, params) => {
      isInWindmill && isThirdGroupAPI ? UserTrack.customAdvance({
        eventId: eventid,
        eventid: eventid,
        name: pageName,
        pageName: pageName,
        arg1: arg1,
        arg2: arg2,
        arg3: arg2,
        param: params,
        params: params
      }) : UserTrack.customAdvance(pageName, eventid, arg1, arg2, arg3, params);
    },
    commit: (weexGmKey, name, comName, params) => {
      isInWindmill && isThirdGroupAPI ? UserTrack.commit({
        type: weexGmKey,
        name: name,
        pageName: name,
        comName: comName,
        param: params,
        params: params
      }) : UserTrack.commit(weexGmKey, name, comName, params);
    },
    enterEvent: (name, params) => {
      isInWindmill && isThirdGroupAPI ? UserTrack.commitut({
        type: "enter",
        eventId: "-1",
        eventid: "-1",
        name: name,
        pageName: name,
        comName: "",
        arg1: "",
        arg2: "",
        arg3: "",
        param: params,
        params: params
      }) : UserTrack.enterEvent ? UserTrack.enterEvent(name, params) : UserTrack.commit("enter", name, "", params);
    },
    updatePageUtparam: paramsStr => {
      isInWindmill && isThirdGroupAPI ? UserTrack.updatePageUtparam({
        utParamJson: paramsStr
      }) : UserTrack.updatePageUtparam(paramsStr);
    },
    updateNextPageUtparam: paramsStr => {
      isInWindmill && isThirdGroupAPI ? UserTrack.updateNextPageUtparam({
        utParamJson: paramsStr
      }) : UserTrack.updateNextPageUtparam(paramsStr);
    },
    commitut: (type, eventid, pageName, comName, arg1, arg2, arg3, params) => {
      isInWindmill && isThirdGroupAPI ? UserTrack.commitut({
        type: type,
        eventId: eventid,
        eventid: eventid,
        name: pageName,
        pageName: pageName,
        comName: comName,
        arg1: arg1,
        arg2: arg2,
        arg3: arg3,
        param: params,
        params: params
      }) : UserTrack.commitut ? UserTrack.commitut(type, eventid, pageName, comName, arg1, arg2, arg3, params) : UserTrack.commit && UserTrack.commit(type, arg1, arg1, params);
    }
  };
  isNode && (global.window = {});
  const DEFAULT_WEEX_GM_KEY = "click";
  const WEEX_GM_KEY_MAP = {
    CLK: "click",
    EXP: "expose",
    OTHER: "other"
  };
  const goldlog = {
    record(logkey, gmkey, gokey, chksum) {
      if (isWeex) {
        let logkeyargs = "string" == typeof gokey ? paramsToObj(gokey) : gokey;
        "undefined" == typeof logkeyargs && (logkeyargs = {});
        logkeyargs.weex = logkeyargs.weex ? logkeyargs.weex : "1";
        logkeyargs.autosend = "1";
        gokey = "string" == typeof gokey ? gokey : objToParams(gokey, true);
        let weexGmKey = WEEX_GM_KEY_MAP[gmkey];
        weexGmKey = weexGmKey || DEFAULT_WEEX_GM_KEY;
        let params = {
          logkey: logkey,
          weex: logkeyargs.weex,
          autosend: logkeyargs.autosend,
          urlpagename: "",
          url: logkeyargs.url || location && location.href || "",
          "spm-cnt": universal_spm_default.a.getSPM().join("."),
          cna: "",
          extendargs: JSON.stringify({}),
          isonepage: 0,
          _lka: JSON.stringify({
            gmkey: gmkey,
            gokey: gokey
          }),
          gokey: gokey
        };
        if (userTrack.commitut) {
          let pageName = logkeyargs.name || params.url;
          let arg1 = logkey;
          let arg2 = "";
          let arg3 = "";
          switch (weexGmKey) {
           case "expose":
            userTrack.commitut(weexGmKey, 2201, pageName, "", arg1, arg2, arg3, params);
            break;

           case "other":
            userTrack.commitut(weexGmKey, 19999, pageName, "", arg1, "", "", params);
            break;

           case "click":
           default:
            userTrack.customAdvance ? userTrack.customAdvance(pageName, 2101, arg1, arg2, arg3, params) : userTrack.commitut(weexGmKey, 2101, pageName, arg1, "", "", "", params);
          }
        } else userTrack.commit && userTrack.commit(weexGmKey, logkey, logkey, params);
      } else if (isWeb && window.goldlog) {
        gokey = "string" == typeof gokey ? gokey : objToParams(gokey, true);
        window.goldlog.record(logkey, gmkey, gokey, chksum);
      }
    },
    launch() {
      let pageSPM = arguments.length > 0 && void 0 !== arguments[0] ? arguments[0] : universal_spm_default.a.getPageSPM();
      let params = arguments.length > 1 && void 0 !== arguments[1] ? arguments[1] : {};
      let checksum = makeChkSum(pageSPM.join("."));
      const logConfig = {
        checksum: checksum,
        is_auto: false,
        page_id: ""
      };
      if (params.page_id) {
        logConfig.page_id = params.page_id;
        delete params.page_id;
      }
      if (isWeex) {
        universal_spm_default.a.setPageSPM(pageSPM[0], pageSPM[1]);
        params.url = params.url || location && location.href || "";
        params["spm-cnt"] = universal_spm_default.a.getPageSPM().join(".") + ".0.0";
        getParamFromURL(params.url, "spm") && (params["spm-url"] = params["spm-url"] || getParamFromURL(params.url, "spm"));
        params.scm = params.scm || getParamFromURL(params.url, "scm") || "0.0.0.0";
        params.cna = "";
        params.weex = 1;
        const name = params.name || simplifyURL(params.url);
        userTrack.enterEvent ? userTrack.enterEvent(name, params) : userTrack.commit && userTrack.commit("enter", name, "", params);
      } else if (isWeb) {
        const waiting = !!getMetaContentByName("aplus-waiting");
        if (waiting) {
          const q = window.goldlog_queue || (window.goldlog_queue = []);
          q.push({
            action: "goldlog.setPageSPM",
            arguments: [ pageSPM[0], pageSPM[1] ]
          });
          q.push({
            action: "goldlog.sendPV",
            arguments: [ logConfig, params ]
          });
        } else window.goldlog && window.goldlog.setPageSPM && window.goldlog.setPageSPM(pageSPM[0], pageSPM[1], () => {
          window.goldlog.sendPV && window.goldlog.sendPV({
            checksum: checksum
          });
        });
      }
    },
    updateNextProps() {
      let params = arguments.length > 0 && void 0 !== arguments[0] ? arguments[0] : {};
      isWeex && "object" === typeof params && userTrack.commitut && userTrack.commitut("updateNextProp", -1, "", "", "", "", "", params);
    },
    updatePageUtparam() {
      let params = arguments.length > 0 && void 0 !== arguments[0] ? arguments[0] : {};
      isWeex && "object" === typeof params && userTrack.updatePageUtparam && userTrack.updatePageUtparam(JSON.stringify(params));
    },
    updateNextPageUtparam() {
      let params = arguments.length > 0 && void 0 !== arguments[0] ? arguments[0] : {};
      isWeex && "object" === typeof params && userTrack.updateNextPageUtparam && userTrack.updateNextPageUtparam(JSON.stringify(params));
    }
  };
  goldlog.sendPV = goldlog.launch;
  var src_goldlog = goldlog;
  var tracker = __webpack_require__(5);
  var tracker_default = __webpack_require__.n(tracker);
  function _objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        _defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function _defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
  __webpack_exports__.default = _objectSpread({}, src_goldlog, tracker_default.a);
}, function(module, __webpack_exports__, __webpack_require__) {
  "use strict";
  __webpack_require__.r(__webpack_exports__);
  const DEFAULT_ERROR_CODE = "rx_user_define_err";
  const errorCodeHash = {
    render: "rx_render_err",
    data: "rx_data_fetch_err",
    error: "rax_error"
  };
  var uri = {
    format(config) {
      const uri = isNode ? "" : location.href || "";
      const errorCode = errorCodeHash[config.type || "custom"] || DEFAULT_ERROR_CODE;
      const pureURI = uri.replace(/[\?#].*$/, "").replace(/\/$/, "");
      const url = [ pureURI, config.module || "", errorCode ].join("/");
      return isWeex ? encodeURIComponent(url) : url;
    }
  };
  let count = 0;
  const defaultConfig = {
    screen: (screen && screen.width) + "x" + (screen && screen.height),
    sampling: 1,
    version: "rx-tracker/2.2.7",
    native: isWeex ? 1 : 0,
    isInWindmill: 0
  };
  const logkey = "/jstracker.3";
  function report(userConfig) {
    if (++count > 20 && isWeb) return false;
    const config = Object.assign({
      url: uri.format(userConfig)
    }, defaultConfig, userConfig);
    config.sampling = isDebug() ? 1 : config.sampling;
    if (Math.random() * config.sampling < 1) if (isWeex) {
      const UserTrack = __webpack_require__(0);
      const weex = "undefined" !== typeof __weex_options__ && __weex_options__.weex;
      const isInWindmill = isWeex && "object" === typeof weex && "windmill" === weex.config.container || false;
      let isThirdGroupAPI = false;
      if (isInWindmill) try {
        let keyboard = __webpack_require__(4);
        typeof keyboard && "function" == typeof keyboard.hideKeyboard && (isThirdGroupAPI = true);
      } catch (e) {}
      if (UserTrack.commitEvent) if (isInWindmill && isThirdGroupAPI) {
        config.isInWindmill = "weex";
        UserTrack.commitEvent({
          eventId: "19999",
          eventid: "19999",
          name: location.href,
          pageName: location.href,
          arg1: logkey,
          arg2: "",
          arg3: "",
          param: config,
          params: config
        });
      } else UserTrack.commitEvent(location.href, "19999", logkey, "", "", config);
    } else if (isWeb && "undefined" !== typeof goldlog) goldlog.send && goldlog.send("//gm.mmstat.com" + logkey, config); else if (isThirdWindmill()) {
      config.isInWindmill = "web";
      sendLogByThirdWindmill(config);
    }
  }
  function isThirdWindmill() {
    return !("undefined" === typeof my || !my.getSystemInfoSync || "function" !== typeof my.getSystemInfoSync || "TB" !== my.getSystemInfoSync().app);
  }
  function sendLogByThirdWindmill(config) {
    my.httpRequest && "function" === typeof my.httpRequest && my.httpRequest({
      url: "//gm.mmstat.com" + logkey,
      method: "get",
      data: config,
      dataType: "json"
    });
  }
  function reportError(error, moduleName, reverse1) {
    let userConfig = {};
    if (error && error instanceof Error) {
      userConfig = {
        stack: JSON.stringify(error.stack),
        name: error.name,
        message: error.message,
        type: "error",
        module: moduleName
      };
      reverse1 && (userConfig.reverse1 = reverse1);
      report(userConfig);
    }
  }
  function reportApi(userConfig, moduleName) {
    if (userConfig && userConfig.url) {
      userConfig.type = "api";
      userConfig.module = moduleName;
      userConfig.sampling = userConfig.sampling || "100";
      report(userConfig);
    }
  }
  function setConfig(config) {
    config && config.sampling && (defaultConfig.sampling = config.sampling);
  }
  function isDebug() {
    const uri = isNode ? "" : location.href;
    return uri.indexOf("jt_debug=1") > -1;
  }
  __webpack_exports__.default = {
    report: report,
    reportError: reportError,
    reportApi: reportApi,
    setConfig: setConfig
  };
} ]);
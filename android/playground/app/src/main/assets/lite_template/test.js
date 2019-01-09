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

const window = {
  console: console,
  document: document,
  open: open
};

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
  __webpack_require__(__webpack_require__.s = 3);
}([ function(module, exports) {
  module.exports = require("@weex-module/userTrack");
}, function(module, exports) {
  module.exports = require("@weex-module/xsearchEvent");
}, function(module, exports) {
  module.exports = require("@weex-module/searchEvent");
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
  const Image = props => {
    const _props$source = props.source, source = void 0 === _props$source ? {} : _props$source, extProps = _objectWithoutProperties(props, [ "source" ]);
    const _ref = source || {}, src = _ref.uri;
    return createElement("image", _extends({
      src: src,
      resizeMode: "cover"
    }, extProps));
  };
  var rax_image = Image;
  function rax_touchable_extends() {
    rax_touchable_extends = Object.assign || function(target) {
      for (var i = 1; i < arguments.length; i++) {
        var source = arguments[i];
        for (var key in source) Object.prototype.hasOwnProperty.call(source, key) && (target[key] = source[key]);
      }
      return target;
    };
    return rax_touchable_extends.apply(this, arguments);
  }
  function rax_touchable_objectWithoutProperties(source, excluded) {
    if (null == source) return {};
    var target = rax_touchable_objectWithoutPropertiesLoose(source, excluded);
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
  function rax_touchable_objectWithoutPropertiesLoose(source, excluded) {
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
  const Touchable = props => {
    const onPress = props.onPress, extProps = rax_touchable_objectWithoutProperties(props, [ "onPress" ]);
    return createElement("div", rax_touchable_extends({}, extProps, {
      onClick: onPress
    }));
  };
  var rax_touchable = Touchable;
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
  const defaultStyles = {
    container: {
      width: 702,
      backgroundColor: "#ffffff",
      borderRadius: 24,
      overflow: "hidden",
      flexDirection: "column",
      paddingBottom: 24
    },
    row: {
      flexDirection: "row",
      alignItems: "center"
    },
    shop: {
      flexDirection: "row",
      alignItems: "center",
      backgroundColor: "#fafafa",
      paddingTop: 20,
      paddingBottom: 20,
      paddingLeft: 24,
      paddingRight: 24,
      overflow: "hidden"
    },
    cardTitle: {
      fontSize: 26,
      color: "#999999"
    },
    moreBenifit: {
      height: 42,
      borderRadius: 21,
      paddingLeft: 16,
      paddingRight: 16,
      backgroundColor: "#ffffff",
      borderWidth: 1,
      borderColor: "#ff5000"
    },
    coupon: {
      height: 44,
      flexDirection: "row",
      alignItems: "center",
      flex: 1,
      justifyContent: "flex-start"
    },
    couponText: {
      fontSize: 26
    },
    seperator: {
      height: 1,
      backgroundColor: "#f2f2f2",
      marginBottom: 20,
      marginTop: 10
    },
    descriptionText: {
      color: "#999999",
      fontSize: 24
    },
    linkIcon: {
      fontSize: 24,
      marginLeft: 4
    }
  };
  const WidgetInListStyle = props => {
    const openCouponFilter = props.openCouponFilter, onCouponHelpClick = props.onCouponHelpClick, onCouponShopClick = props.onCouponShopClick, _props$data = props.data, moreBenefit = _props$data.moreBenefit, title = _props$data.title, _props$data$buttonTit = _props$data.buttonTitle, buttonTitle = void 0 === _props$data$buttonTit ? "" : _props$data$buttonTit, _props$data$couponLis = _props$data.couponList, couponList = void 0 === _props$data$couponLis ? [] : _props$data$couponLis, linkTitle = _props$data.linkTitle, linkUrl = _props$data.linkUrl, shopName = _props$data.shopName, shopUrl = _props$data.shopUrl, desc = _props$data.desc;
    const couponNodes = [];
    Array.isArray(couponList) && couponList.forEach(coupon => {
      const iconUrl = coupon.icon, couponTitle = coupon.title, selected = coupon.selected, _coupon$params = coupon.params, params = void 0 === _coupon$params ? [] : _coupon$params;
      const couponData = {
        icon: iconUrl,
        title: couponTitle,
        selected: selected,
        params: params
      };
      const iconSizeArray = iconUrl ? iconUrl.match(/-[0-9]+-[0-9]+\./g) : [];
      let scale = -1;
      if (iconSizeArray.length) {
        let iconSizeStr = iconSizeArray[0];
        if (iconSizeStr) {
          iconSizeStr = iconSizeStr.slice(1, iconSizeStr.length - 1);
          if (2 === iconSizeStr.split("-").length) {
            couponData.width = parseFloat(iconSizeStr.split("-")[0]);
            couponData.height = parseFloat(iconSizeStr.split("-")[1]);
            scale = couponData.height / 24 > 0 ? couponData.height / 24 : 1;
          }
        }
      }
      let imageWidth = scale > 0 ? couponData.width / scale : 0;
      couponNodes.push(createElement(rax_view, {
        style: _objectSpread({}, defaultStyles.coupon, {
          paddingLeft: 24,
          paddingRight: 24
        })
      }, createElement(rax_image, {
        source: {
          uri: iconUrl
        },
        resizeMode: "contain",
        style: {
          height: 24,
          width: imageWidth,
          marginRight: scale > 0 ? 10 : 0
        }
      }), createElement(rax_text, {
        style: _objectSpread({}, defaultStyles.couponText, {
          maxWidth: 702 - imageWidth - 40
        }),
        numberOfLines: 1
      }, couponTitle)));
    });
    const onShopClick = () => {
      "function" === typeof onCouponShopClick && onCouponShopClick();
      window.open(shopUrl);
    };
    const onHeplClick = () => {
      "function" === typeof onCouponHelpClick && onCouponHelpClick();
      window.open(linkUrl);
    };
    const shopNode = shopName && shopUrl ? createElement(rax_touchable, {
      style: _objectSpread({}, defaultStyles.shop),
      onPress: onShopClick
    }, createElement(rax_view, {
      style: {
        flexDirection: "row",
        flex: 1
      }
    }, createElement(rax_image, {
      style: {
        height: 32,
        width: 32,
        marginRight: 5
      },
      source: {
        uri: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAhFBMVEUAAAAzMzMzMzMzMzMzMzMzMzMzMzMzMzMyMjIyMjIzMzMzMzMyMjIyMjIzMzMyMjIyMjIzMzMxMTEzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMzMyMjIzMzM0NDQzMzMzMzMzMzMzMzM0NDQyMjIyMjIzMzMzMzMzMzOGeldaAAAAK3RSTlMAVar6798P06ePdq5mSEE1GxIH5KOdhnMm8+nJxbR6amBYLOrZx7yYk3xuarL8WgAAANNJREFUOMvVkEkOwjAMRR2cNE06t7Rlnmff/36IJMCiicQKwdt8W3qyvgy/QoHkQb2FWLIhK5yAo8INeDiOn9OK1z4hp8JNQoGX09JmRpNAd8pNLgULcFgYASlM9BCIQYCcKiNs7bqLwTFPXTt7oensqp9la3Sq5iYULx8RCXKfUYSmfjk7mz2S/JplNyE2KPWW9QtkCbbrVHFZgTU6QdRcKtiNG6JZUgCs53suuwg+RZfgodQmwp9g9F/CNPUJ6fQ1tjwZDUh4+xLqPh4KcV/Dd7gDnwsUwu/EmYQAAAAASUVORK5CYII="
      }
    }), createElement(rax_text, {
      style: _objectSpread({}, defaultStyles.cardTitle, {
        fontSize: 24,
        color: "#666666",
        flex: 1,
        maxWidth: 500
      }),
      numberOfLines: 1
    }, shopName)), createElement(rax_text, {
      style: _objectSpread({}, defaultStyles.descriptionText, {
        fontSize: 24,
        marginLeft: 5
      }),
      numberOfLines: 1
    }, "\u8fdb\u5e97"), createElement(rax_image, {
      style: {
        height: 24,
        width: 24,
        resizeMode: "contain"
      },
      source: {
        uri: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAYFBMVEUAAACZmZmZmZmZmZmZmZmZmZmampqXl5eZmZmZmZmZmZmZmZmampqZmZmampqXl5eampqampqZmZmYmJiZmZmZmZmZmZmZmZmZmZmXl5eZmZmZmZmZmZmampqZmZmZmZkTMmvtAAAAH3RSTlMA+ALx4MxaBunXwXYuIxMNq5ePi4JpSz85G7axmFMZXgU88gAAAH5JREFUOMvN0lcOgDAMA1Bo2XtTZu5/S9IL2BIfiP76qUpTB98eQ/JTekhMKNKlSAwisiNhRhVFi+4oVcQNErOKfEMi0UmjmomsQmKhYrUi1iHhvFiRqDKdlI0Rg/wiT014rssky77Zd73PJ1wIM5DC+EIdqHKWlXYKtfa/PA9xeQpg3vtK9AAAAABJRU5ErkJggg=="
      }
    })) : null;
    const titleNode = createElement(rax_text, {
      style: _objectSpread({}, defaultStyles.cardTitle, {
        maxWidth: 400
      }),
      numberOfLines: 1
    }, title);
    const moreBenefitNode = buttonTitle && moreBenefit ? createElement(rax_touchable, {
      style: _objectSpread({}, defaultStyles.moreBenifit, defaultStyles.row),
      onPress: openCouponFilter
    }, createElement(rax_text, {
      style: _objectSpread({}, defaultStyles.cardTitle, {
        color: "#ff5000",
        maxWidth: 180
      }),
      numberOfLines: 1
    }, buttonTitle)) : null;
    return createElement(rax_view, {
      style: _objectSpread({}, defaultStyles.container)
    }, shopNode, createElement(rax_view, {
      style: _objectSpread({}, defaultStyles.row, {
        justifyContent: "space-between",
        marginBottom: 10,
        marginTop: 24,
        paddingLeft: 24,
        paddingRight: 24
      })
    }, titleNode, moreBenefitNode), couponNodes, createElement(rax_view, {
      style: _objectSpread({}, defaultStyles.seperator, defaultStyles.flex)
    }), createElement(rax_view, {
      style: _objectSpread({
        flex: 1
      }, defaultStyles.black, defaultStyles.row, defaultStyles.descriptionText, {
        justifyContent: "space-between",
        paddingLeft: 24,
        paddingRight: 24
      })
    }, createElement(rax_text, {
      style: _objectSpread({}, defaultStyles.black, defaultStyles.descriptionText, {
        maxWidth: 560,
        textOverflow: "hidden"
      }),
      numberOfLines: 1
    }, desc), createElement(rax_touchable, {
      style: _objectSpread({}, defaultStyles.row),
      onPress: onHeplClick
    }, createElement(rax_text, {
      style: _objectSpread({}, defaultStyles.descriptionText, {
        maxWidth: 120,
        textOverflow: "hidden"
      }),
      numberOfLines: 1
    }, linkTitle), createElement(rax_image, {
      style: {
        height: 24,
        width: 24,
        marginLeft: 5
      },
      source: {
        uri: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABgAAAAYCAMAAADXqc3KAAAAXVBMVEUAAACZmZmampqZmZmYmJiZmZmZmZmZmZmZmZmZmZmZmZmYmJiZmZmZmZmZmZmZmZmZmZmZmZmampqZmZmZmZmZmZmYmJigoKCYmJiYmJiYmJiampqZmZmYmJiZmZlqoIC3AAAAHnRSTlMAcirdEr/3aDfbWQ4G2FGxqOHFY15BJwltRUONjHehq3KPAAAA20lEQVQoz4VS7XLDIAyrMYSST9Kkaddtev/HXK2EZtntrvph62xsYcPpPVwtUru/0ZskEEna3/HziBBd07gYoOc93lfpWvg1Vf3rfHXxTzfnPD+dv1RbzW1MFh+gI7JlVFcdgfWZEI1azQPCRApmh86sDmYDuUM0N03smo1H2Dw1LdEOq65DTYmmJDIm+gZySMz4XomH7K2K/t6K4kTg9Si+lOsS93s50b0GPOBjG7DV5Ek+v8xyJYclesBvS9zXrg9zvfXRsnbW6H8PRR3pQHR82gMW+wzL6S1+ALp1C37C4N5EAAAAAElFTkSuQmCC"
      }
    }))));
  };
  WidgetInListStyle.displayName = "WidgetInListStyle";
  var CouponCard = WidgetInListStyle;
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
  function commonClick(controlName) {
    let trace = arguments.length > 1 && void 0 !== arguments[1] ? arguments[1] : {};
    let status = arguments.length > 2 && void 0 !== arguments[2] ? arguments[2] : {};
    let newTrace = {};
    try {
      newTrace = processListParam(JSON.parse(JSON.stringify(trace)));
    } catch (error) {}
    const index = status.index, pageName = status.pageName;
    const _trace$utLogMap2 = trace.utLogMap, utLogMap = void 0 === _trace$utLogMap2 ? {} : _trace$utLogMap2;
    const newUtLogMap = ut_objectSpread({}, utLogMap, {
      index: index
    });
    newTrace.utLogMap = newUtLogMap;
    Object.keys(newTrace).forEach(key => {
      "object" === typeof newTrace[key] && (newTrace[key] = encodeURIComponent(JSON.stringify(newTrace[key])));
    });
    const _status$hubbleInfo2 = status.hubbleInfo, hubbleInfo = void 0 === _status$hubbleInfo2 ? {} : _status$hubbleInfo2;
    const args = ut_objectSpread({
      hubbleTemplateInfo: encodeURIComponent(JSON.stringify(hubbleInfo))
    }, newTrace);
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
    const xsearchEvent = __webpack_require__(1);
    const userTrack = __webpack_require__(0);
    if (xsearchEvent && "function" === typeof xsearchEvent.updateStorage) {
      xsearchEvent.updateStorage(newStorage);
      userTrack.commitut("expose", -1, pageName, "Button-" + controlName, pageName + "_Button-" + controlName, "", "", exposure_objectSpread({}, newTrace, extraParams, {
        hubbleTemplateInfo: encodeURIComponent(JSON.stringify(hubbleInfo))
      }));
    }
  }
  function commonExpose(controlName) {
    let trace = arguments.length > 1 && void 0 !== arguments[1] ? arguments[1] : {};
    let state = arguments.length > 2 && void 0 !== arguments[2] ? arguments[2] : {};
    expose(controlName, trace, state);
  }
  function lib_objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        lib_defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function lib_defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
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
  const _searchEvent = __webpack_require__(2);
  const defaultStyle = {
    container: {
      padding: 24
    }
  };
  const Widget = props => {
    const _props$model = props.model, model = void 0 === _props$model ? {} : _props$model, _props$status = props.status, status = void 0 === _props$status ? {} : _props$status, _props$trace = props.trace, trace = void 0 === _props$trace ? {} : _props$trace;
    let _props$state = props.state, state = void 0 === _props$state ? {
      data: {
        status: {
          pageName: "Page_CouponUseSrp"
        }
      }
    } : _props$state;
    const _model$data = model.data, _model$data2 = void 0 === _model$data ? {} : _model$data, moreBenefit = _model$data2.moreBenefit, _model$data2$couponLi = _model$data2.couponList, couponList = void 0 === _model$data2$couponLi ? [] : _model$data2$couponLi, buttonTitle = _model$data2.buttonTitle;
    state && state.data && (state.data.status ? state.data.status.pageName = "Page_SearchItemList" : state.data.status = {
      pageName: "Page_SearchItemList"
    });
    trace.couponCount = couponList.length;
    trace.hasMoreBenefit = moreBenefit && buttonTitle ? 1 : 0;
    commonExpose("couponTipsExpose", trace, state, true);
    const openCouponFilter = () => {
      _searchEvent.openCouponDynamicFilter(moreBenefit);
      commonClick("openCouponFilter", trace, status);
    };
    const onCouponHelpClick = () => {
      commonClick("openCouponHelp", trace, status);
    };
    const onCouponShopClick = () => {
      commonClick("openCouponShop", trace, status);
    };
    const content = createElement(CouponCard, lib_extends({}, model, {
      openCouponFilter: openCouponFilter,
      onCouponHelpClick: onCouponHelpClick,
      onCouponShopClick: onCouponShopClick
    }));
    return createElement(rax_view, {
      style: lib_objectSpread({}, defaultStyle.container)
    }, content);
  };
  var lib = Widget;
  const dataParser = data => {
    let parsedData = data;
    return parsedData;
  };
  function src_extends() {
    src_extends = Object.assign || function(target) {
      for (var i = 1; i < arguments.length; i++) {
        var source = arguments[i];
        for (var key in source) Object.prototype.hasOwnProperty.call(source, key) && (target[key] = source[key]);
      }
      return target;
    };
    return src_extends.apply(this, arguments);
  }
  function src_objectSpread(target) {
    for (var i = 1; i < arguments.length; i++) {
      var source = null != arguments[i] ? arguments[i] : {};
      var ownKeys = Object.keys(source);
      "function" === typeof Object.getOwnPropertySymbols && (ownKeys = ownKeys.concat(Object.getOwnPropertySymbols(source).filter(function(sym) {
        return Object.getOwnPropertyDescriptor(source, sym).enumerable;
      })));
      ownKeys.forEach(function(key) {
        src_defineProperty(target, key, source[key]);
      });
    }
    return target;
  }
  function src_defineProperty(obj, key, value) {
    key in obj ? Object.defineProperty(obj, key, {
      value: value,
      enumerable: true,
      configurable: true,
      writable: true
    }) : obj[key] = value;
    return obj;
  }
  const _data = __weex_data__;
  const src_defaultStyle = {
    container: {}
  };
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
      const _data$trace = data.trace, trace = void 0 === _data$trace ? {} : _data$trace;
      const parsedData = dataParser(data);
      return createElement(rax_view, {
        style: src_objectSpread({}, src_defaultStyle.container)
      }, createElement(lib, src_extends({}, parsedData, {
        trace: trace,
        state: this.state
      })));
    }
  }
  render(createElement(src_Segment, null));
} ]);
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
  __webpack_require__(__webpack_require__.s = 2);
}([ function(module, exports) {
  module.exports = require("@weex-module/searchEvent");
}, function(module, exports) {
  module.exports = require("@weex-module/userTrack");
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
  const NXImage = props => createElement("image", _extends({
    resizeMode: "cover"
  }, props));
  var nx_image = NXImage;
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
  const Touchable = props => {
    const onPress = props.onPress, extProps = _objectWithoutProperties(props, [ "onPress" ]);
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
  const style = {
    container: {
      flexDirection: "column",
      paddingLeft: 24,
      paddingRight: 24,
      paddingTop: 40,
      width: 750,
      backgroundColor: "#ffffff",
      paddingBottom: 12
    },
    titleContainer: {
      flexDirection: "row",
      alignItems: "center"
    },
    titleIcon: {
      width: 25,
      height: 25
    },
    titleText: {
      marginLeft: 10,
      fontSize: 26,
      color: "#666666"
    },
    tagContainer: {
      flexWrap: "wrap",
      flexDirection: "row",
      justifyContent: "space-between"
    },
    tagItem: {
      width: 340,
      height: 92,
      flexDirection: "row",
      alignItems: "center",
      marginTop: 18,
      paddingLeft: 42,
      paddingRight: 20
    },
    tagIcon: {
      width: 66,
      height: 66,
      borderRadius: 6
    },
    tagBg: {
      width: 340,
      height: 92,
      backgroundColor: "#000000",
      opacity: .02,
      borderRadius: 46,
      position: "absolute",
      top: 0,
      left: 0
    },
    tagText: {
      color: "#333333",
      fontSize: 26,
      marginLeft: 30,
      textOverflow: "clip",
      flex: 1
    }
  };
  const WidgetInListStyle = props => {
    const topic = props.topic, tips = props.tips, onClick = props.onClick;
    let tags = tips.slice(0, 4).map((item, index) => createElement(rax_touchable, {
      style: _objectSpread({}, style.tagItem),
      onPress: () => {
        "function" === typeof onClick && onClick(item, index);
      }
    }, createElement(rax_view, {
      style: _objectSpread({}, style.tagIcon)
    }, createElement(nx_image, {
      style: _objectSpread({}, style.tagIcon),
      src: item.imgUrl
    })), createElement(rax_text, {
      style: _objectSpread({}, style.tagText),
      numberOfLines: "1"
    }, item.show), createElement(rax_view, {
      style: _objectSpread({}, style.tagBg)
    })));
    return createElement(rax_view, {
      style: _objectSpread({}, style.container)
    }, createElement(rax_view, {
      style: _objectSpread({}, style.titleContainer)
    }, createElement(nx_image, {
      style: _objectSpread({}, style.titleIcon),
      src: "https://gw.alicdn.com/tfs/TB1BCvxh9zqK1RjSZFpXXakSXXa-25-25.png"
    }), createElement(rax_text, {
      style: _objectSpread({}, style.titleText)
    }, topic || "\u4f60\u662f\u4e0d\u662f\u60f3\u627e")), createElement(rax_view, {
      style: _objectSpread({}, style.tagContainer)
    }, tags));
  };
  WidgetInListStyle.displayName = "WidgetInListStyle";
  var lib_WidgetInListStyle = WidgetInListStyle;
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
  const WidgetInWFStyle_style = {
    container: {
      marginTop: 18,
      flexDirection: "column",
      paddingLeft: 24,
      paddingRight: 24,
      paddingTop: 30,
      paddingBottom: 30,
      width: 342,
      backgroundColor: "#ffffff",
      borderRadius: 12
    },
    titleContainer: {
      flexDirection: "row",
      alignItems: "center"
    },
    titleIcon: {
      width: 25,
      height: 25
    },
    titleText: {
      marginLeft: 10,
      fontSize: 26,
      color: "#666666"
    },
    tagContainer: {
      flexWrap: "wrap",
      flexDirection: "row"
    },
    tagItem: {
      width: 294,
      height: 92,
      flexDirection: "row",
      alignItems: "center",
      marginTop: 18,
      paddingLeft: 28
    },
    tagBg: {
      width: 294,
      height: 92,
      backgroundColor: "#000000",
      opacity: .02,
      borderRadius: 46,
      position: "absolute",
      top: 0,
      left: 0
    },
    tagIcon: {
      width: 72,
      height: 72,
      borderRadius: 8
    },
    tagText: {
      color: "#333333",
      marginLeft: 12,
      fontSize: 26,
      flex: 1
    }
  };
  const WidgetInWFStyle = props => {
    const topic = props.topic, tips = props.tips, onClick = props.onClick;
    let tags = tips.slice(0, 4).map((item, index) => createElement(rax_touchable, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.tagItem),
      onPress: () => {
        "function" === typeof onClick && onClick(item, index);
      }
    }, createElement(rax_view, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.tagIcon)
    }, createElement(nx_image, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.tagIcon),
      src: item.imgUrl
    })), createElement(rax_text, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.tagText),
      numberOfLines: "1"
    }, item.show), createElement(rax_view, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.tagBg)
    })));
    return createElement(rax_view, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.container)
    }, createElement(rax_view, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.titleContainer)
    }, createElement(nx_image, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.titleIcon),
      src: "https://gw.alicdn.com/tfs/TB1BCvxh9zqK1RjSZFpXXakSXXa-25-25.png"
    }), createElement(rax_text, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.titleText)
    }, topic || "\u4f60\u662f\u4e0d\u662f\u60f3\u627e")), createElement(rax_view, {
      style: WidgetInWFStyle_objectSpread({}, WidgetInWFStyle_style.tagContainer)
    }, tags));
  };
  WidgetInWFStyle.displayName = "WidgetInWFStyle";
  var lib_WidgetInWFStyle = WidgetInWFStyle;
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
  const Widget = props => {
    const _props$model = props.model, model = void 0 === _props$model ? {} : _props$model, _props$status = props.status, status = void 0 === _props$status ? {} : _props$status, onClick = props.onClick;
    const _status$layoutStyle = status.layoutStyle, layoutStyle = void 0 === _status$layoutStyle ? 0 : _status$layoutStyle;
    const content = 1 === parseInt(layoutStyle, 10) ? createElement(lib_WidgetInWFStyle, lib_extends({}, model, {
      onClick: onClick
    })) : createElement(lib_WidgetInListStyle, lib_extends({}, model, {
      onClick: onClick
    }));
    return createElement(rax_view, null, content);
  };
  var lib = Widget;
  const _data = __weex_data__;
  const searchEvent = __webpack_require__(0);
  const userTrack = __webpack_require__(1);
  let exposed = false;
  class src_Segment extends Component {
    constructor(props) {
      super(props);
      this.onClick = ((tag, index) => {
        const _this$state$data = this.state.data, data = void 0 === _this$state$data ? {} : _this$state$data;
        const model = data.model, status = data.status;
        const layoutStyle = status.layoutStyle, hubbleInfo = status.hubbleInfo;
        const rn = model.rn, itemId = model.itemId, cardRn = model.cardRn;
        let params = {};
        if (model.src && "botSearch3" === model.src) {
          params.jh_source = "srpTips";
          params.jh_op = "select";
          status && status.keyword && (params.q = status.keyword);
          let onFilter = "";
          Array.isArray(tag.params) && tag.params.length && tag.params.forEach(item => {
            params[item.key] = item.value;
            "onFilter" === item.key && (onFilter = item.value);
          });
          !model.allFilters && onFilter ? params.allFilters = onFilter : model.allFilters && onFilter && (params.allFilters = model.allFilters + ";" + onFilter);
        } else {
          Array.isArray(tag.params) && tag.params.length && tag.params.forEach(param => {
            params[param.key] = param.value;
          });
          tag.q && (params.q = tag.q);
          searchEvent.utClickLog({
            controlName: "RealTimeTag",
            args: {
              pos: index,
              query: tag.show,
              rn: rn,
              item_id: itemId,
              cardRn: cardRn,
              style: 0 === layoutStyle ? "list" : "wf",
              tItemType: "wx_realtime_tag_image",
              hubbleTemplateInfo: encodeURIComponent(JSON.stringify(hubbleInfo))
            }
          });
        }
        searchEvent.search({
          params: params
        });
      });
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
      const _this$state$data2 = this.state.data, data = void 0 === _this$state$data2 ? {} : _this$state$data2;
      const status = data.status, model = data.model;
      const hubbleInfo = status.hubbleInfo, layoutStyle = status.layoutStyle;
      const itemId = model.itemId, q = model.q, rn = model.rn, bucketId = model.bucketId, position = model.position, tips = model.tips, cardRn = model.cardRn;
      data.onClick = this.onClick;
      let exposeArg1 = model.src && "botSearch3" === model.src ? "Page_Search_RecommendcardShow_tag" : "Page_Search_RecommendcardShow_wx";
      return createElement(rax_view, null, createElement(lib, data));
    }
  }
  render(createElement(src_Segment, null));
} ]);
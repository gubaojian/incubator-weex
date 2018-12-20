{
  "body": {
    "tagName": "div",
    "nodeId": "0",
    "classList": [
      "screen"
    ],
    "childNodes": [
      {
        "tagName": "div",
        "nodeId": "1",
        "style": {
          "alignItems": "center"
        },
        "childNodes": [
          {
            "tagName": "text",
            "nodeId": "2",
            "classList": [
              "title"
            ],
            "attributes": {
              "value": "Klotski (华容道)"
            }
          },
          {
            "tagName": "text",
            "nodeId": "3",
            "classList": [
              "subtitle"
            ],
            "attributes": {
              "value": "A classic sliding pulzze game in China."
            }
          }
        ]
      },
      {
        "tagName": "div",
        "nodeId": "4",
        "style": {
          "position": "relative"
        },
        "childNodes": [
          {
            "tagName": "div",
            "nodeId": "5",
            "classList": [
              "board"
            ],
            "style": [{
              "@binding": "wrapperStyle"
            }],
            "childNodes": [
              {
                "tagName": "div",
                "nodeId": "6",
                "control": {
                  "repeat": {
                    "for": "blocks",
                    "alias": "block",
                    "iterator1": "i"
                  }
                },
                "attributes": {
                  "key": {
                    "@binding": "i"
                  },
                  "@swipe": "move(block, $event)"
                },
                "classList": [
                  "block",
                  {
                    "@binding": "block.isBoss ? \"block-boss\" : \"block-normal\""
                  }
                ],
                "style": [{
                  "@binding": "blockStyle[i]"
                }],
                "event": [
                  {
                    "type": "swipe",
                    "params": [
                      {
                        "@binding": "block"
                      },
                      {
                        "@binding": "$event"
                      }
                    ]
                  }
                ],
                "childNodes": [
                  {
                    "tagName": "text",
                    "nodeId": "7",
                    "classList": [
                      "name"
                    ],
                    "attributes": {
                      "v-once": "",
                      "value": {
                        "@binding": "block.name"
                      }
                    }
                  }
                ]
              },
              {
                "tagName": "div",
                "nodeId": "8",
                "classList": [
                  "result"
                ],
                "control": {
                  "match": "this.isWin"
                },
                "childNodes": [
                  {
                    "tagName": "text",
                    "nodeId": "9",
                    "classList": [
                      "win"
                    ],
                    "attributes": {
                      "value": "You Win !"
                    }
                  }
                ]
              }
            ]
          },
          {
            "tagName": "div",
            "nodeId": "10",
            "classList": [
              "stick"
            ],
            "style": [{
              "@binding": "stickStyle"
            }]
          },
          {
            "tagName": "div",
            "nodeId": "11",
            "classList": [
              "operations"
            ],
            "style": {
              "width": {
                "@binding": "meshSize * 4 + gap * 2 + \"px\""
              }
            },
            "childNodes": [
              {
                "tagName": "text",
                "nodeId": "12",
                "classList": [
                  "button",
                  "label"
                ],
                "event": [
                  "click"
                ],
                "attributes": {
                  "@click": "archive",
                  "value": "Archive"
                }
              },
              {
                "tagName": "text",
                "nodeId": "13",
                "classList": [
                  "button",
                  "step"
                ],
                "attributes": {
                  "value": {
                    "@binding": "step"
                  }
                }
              },
              {
                "tagName": "text",
                "nodeId": "14",
                "classList": [
                  "button",
                  "label"
                ],
                "event": [
                  "click"
                ],
                "attributes": {
                  "@click": "reset",
                  "value": "Reset"
                }
              }
            ]
          }
        ]
      },
      {
        "tagName": "div",
        "nodeId": "15",
        "style": {
          "marginBottom": "20px"
        },
        "childNodes": [
          {
            "tagName": "text",
            "nodeId": "16",
            "classList": [
              "tips"
            ],
            "attributes": {
              "value": "Slide the blocks, move the largest piece to the center bottom and you win!"
            }
          }
        ]
      }
    ]
  },
  "data": {
    "step": 0,
    "blocks": [
      {
        "origin": [
          0,
          0
        ],
        "size": [
          1,
          2
        ],
        "name": "张\n飞"
      },
      {
        "origin": [
          1,
          0
        ],
        "size": [
          2,
          2
        ],
        "name": "曹操",
        "isBoss": true
      },
      {
        "origin": [
          3,
          0
        ],
        "size": [
          1,
          2
        ],
        "name": "赵\n云"
      },
      {
        "origin": [
          0,
          2
        ],
        "size": [
          1,
          2
        ],
        "name": "马\n超"
      },
      {
        "origin": [
          1,
          2
        ],
        "size": [
          2,
          1
        ],
        "name": "关羽"
      },
      {
        "origin": [
          3,
          2
        ],
        "size": [
          1,
          2
        ],
        "name": "黄\n忠"
      },
      {
        "origin": [
          1,
          3
        ],
        "size": [
          1,
          1
        ],
        "name": "卒"
      },
      {
        "origin": [
          0,
          4
        ],
        "size": [
          1,
          1
        ],
        "name": "卒"
      },
      {
        "origin": [
          2,
          3
        ],
        "size": [
          1,
          1
        ],
        "name": "卒"
      },
      {
        "origin": [
          3,
          4
        ],
        "size": [
          1,
          1
        ],
        "name": "卒"
      }
    ]
  },
  "components": [],
  "styles": {
    "screen": {
      "justifyContent": "space-around",
      "alignItems": "center"
    },
    "title": {
      "fontSize": "60px",
      "color": "rgb(214, 111, 30)",
      "textAlign": "center",
      "fontWeight": "bold"
    },
    "subtitle": {
      "fontSize": "26px",
      "color": "rgba(214, 111, 30, 0.8)",
      "textAlign": "center"
    },
    "tips": {
      "width": "600px",
      "fontSize": "30px",
      "color": "rgba(214, 111, 30, 0.8)",
      "textAlign": "center"
    },
    "board": {
      "position": "relative",
      "borderColor": "#B37E47",
      "backgroundColor": "#EEDDCC"
    },
    "stick": {
      "position": "absolute",
      "backgroundColor": "#EEDDCC"
    },
    "result": {
      "position": "absolute",
      "backgroundColor": "rgba(26, 25, 31, 0.75)",
      "top": "0",
      "bottom": "0",
      "left": "0",
      "right": "0",
      "justifyContent": "center",
      "alignItems": "center"
    },
    "win": {
      "fontSize": "100px",
      "fontWeight": "bold",
      "color": "#FF6666",
      "textAlign": "center"
    },
    "block": {
      "position": "absolute",
      "flexDirection": "column",
      "justifyContent": "center",
      "alignItems": "center",
      "borderStyle": "solid",
      "borderRadius": "10px",
      "boxShadow": "0 2px 6px rgba(0, 0, 0, 0.3)"
    },
    "block-normal": {
      "borderColor": "#B37E47",
      "backgroundColor": "#D5A471"
    },
    "block-boss": {
      "borderColor": "#BB3D3D",
      "backgroundColor": "#DE6457"
    },
    "operations": {
      "flexDirection": "row",
      "justifyContent": "space-between"
    },
    "button": {
      "padding": "10px",
      "margin": "10px",
      "width": "140px",
      "textAlign": "center"
    },
    "label": {
      "color": "#FFF",
      "fontSize": "30px",
      "borderWidth": "4px",
      "borderRadius": "8px",
      "borderColor": "#DE6457",
      "backgroundColor": "#EA7E72"
    },
    "step": {
      "fontSize": "42px",
      "color": "#DE6457"
    },
    "name": {
      "textAlign": "center",
      "fontSize": "50px",
      "color": "#FFEFE0"
    }
  },
  "props": {
    "meshSize": 150,
    "borderWidth": 6,
    "gap": 5
  },
  "computed": {
    "wrapperStyle": "",
    "blockStyle": "",
    "stickStyle": "",
    "isWin": ""
  },
  "script": [{
    "src":"http://30.5.37.114:8080/demo1.js"
  }]
}
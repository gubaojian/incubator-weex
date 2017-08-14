'use strict';

var _ = require('macaca-utils');
var assert = require('chai').assert
var wd = require('weex-wd')
var path = require('path');
var os = require('os');
var util = require("../util.js");

describe('weex list onrefresh header test', function () {
  this.timeout(util.getTimeoutMills());
  var driver = util.createDriver(wd);

  before(function () {
    return util.init(driver)
      .get(util.getPage('/components/list-onrefresh-header-1px.js'))
      .waitForElementById("list", util.getGETActionWaitTimeMills(), 2000)
  });

  after(function () {
    return util.quit(driver);
  })

  it('#1 list component view event', () => {
    return driver
  })
});

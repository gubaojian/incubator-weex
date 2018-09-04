package com.taobao.weex.ui.component.document;

import com.taobao.weex.WXSDKInstance;
import com.taobao.weex.ui.action.BasicGraphicAction;
import com.taobao.weex.ui.component.WXVContainer;

/**
 * Created by furture on 2018/9/4.
 */

public class InitDocumentViewAction extends BasicGraphicAction {

    private WXDocumentComponent documentComponent;


    public InitDocumentViewAction(WXDocumentComponent documentComponent) {
        super(documentComponent.getInstance(), documentComponent.getRef());
        this.documentComponent = documentComponent;
    }

    @Override
    public void executeAction() {
        documentComponent.lazy(false);
        WXVContainer parent = documentComponent.getParent();
        if(parent != null){
            int index = parent.indexOf(documentComponent);
            parent.createChildViewAt(index);
        }
        documentComponent.applyLayoutAndEvent(documentComponent);
        documentComponent.bindData(documentComponent);
    }
}

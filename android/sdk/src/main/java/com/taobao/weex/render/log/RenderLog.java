package com.taobao.weex.render.log;

import android.util.Log;

import com.taobao.weex.render.view.DocumentView;

import org.json.JSONObject;

import java.util.Collection;
import java.util.Map;

/**
 * Render Log for recording render command and replay render action command
 * Created by furture on 2018/9/1.
 */
public class RenderLog {

    private static final String RENDER_LOG_TAG  = "WeexRenderLog";

    private static  boolean isLogEnabled = false;

    public static final void openRenderLog(boolean enabled){
        isLogEnabled = enabled;
    }


    public static void actionCreateBody(DocumentView documentView, String ref, Map<String, String> style, Map<String, String> attrs, Collection<String> events) {
       if(isLogEnabled){
           Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentActionCreateBody;" + ref + ";"+ toJSON(style) + ";" + toJSON(attrs)
                   + ";" + toJSON(events));
       }
    }

    public static void actionAddElement(DocumentView documentView, String ref, String componentType, String parentRef, int index, Map<String, String> style, Map<String, String> attrs, Collection<String> events){
        if(isLogEnabled){
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentActionAddElement;" + ref + ";" + componentType + ";" + parentRef + ";" + index + ";"
                    + toJSON(style) + ";" + toJSON(attrs)
                    + ";" + toJSON(events));
        }
    }


    public static void actionUpdateStyles(DocumentView documentView, String ref, Map<String, String> styles) {
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentActionUpdateStyle;" + ref + ";" + toJSON(styles));
        }
    }

    public static void actionUpdateAttrs(DocumentView documentView, String ref, Map<String, String> attrs){
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentActionUpdateAttrs;" + ref + ";" + toJSON(attrs));
        }
    }


    public static void actionAddEvent(DocumentView documentView, String ref, String event){
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentActionAddEvent;" + ref + ";" + event);
        }
    }

    public static void actionRemoveEvent(DocumentView documentView, String ref, String event){
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentRemoveEvent;" + ref + ";" + event);
        }
    }


    public static void actionMoveElement(DocumentView documentView, String ref, String parentRef, int index){
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentActionMoveElement;" + ref + ";" + parentRef + ";" + index);
        }
    }


    public static void actionRemoveElement(DocumentView documentView, String ref){
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentActionRemoveElement;" + ref);
        }
    }

    public static void actionLayoutExecute(DocumentView documentView){
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentActionLayoutExecute;" + documentView.getNativeDocument());
        }
    }

    public static void actionInvalidate(DocumentView documentView){
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentInvalidate;" + documentView.getNativeDocument());
        }
    }

    public static void actionSwap(DocumentView documentView){
        if(isLogEnabled) {
            Log.e(RENDER_LOG_TAG, RENDER_LOG_TAG + " " + documentView.hashCode() + "documentSwap;" + documentView.getNativeDocument());
        }
    }

    private static final String toJSON(Map<String, String> style){
        if(style == null){
            return "null";
        }
        return new JSONObject(style).toString();
    }

    private static String toJSON(Collection<String> events) {
        if(events == null){
            return "null";
        }
        return new org.json.JSONArray(events).toString();
    }

}

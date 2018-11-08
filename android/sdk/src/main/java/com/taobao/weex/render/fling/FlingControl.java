package com.taobao.weex.render.fling;

import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.AbsListView;
import android.widget.HorizontalScrollView;
import android.widget.ScrollView;

import java.lang.reflect.Field;


public class FlingControl {


    /**
     * return velocity between[-maxFlingVelocity, maxFlingVelocity]
     * */
    public static int getVelocity(int velocity, int maxFlingVelocity){
        if(velocity > maxFlingVelocity){
            velocity = maxFlingVelocity;
        }
        if(velocity < -maxFlingVelocity){
            velocity = -maxFlingVelocity;
        }
        return velocity;
    }

    /**
     * auto slow down max fling speed.
     * */
    public static void setParentMaxFling(View view){
        View parent = view;
        while (parent != null){
            if(parent instanceof MaxFlingVelocity){
                ((MaxFlingVelocity) parent).setMaxFlingVelocityEnabled(true);
                break;
            }
            if(parent instanceof RecyclerView){
                setRecycleViewMaxFlingVelocity((RecyclerView) parent);
                break;
            }
            if(parent instanceof ScrollView){
                setScrollViewMaxFlingVelocity((ScrollView) parent);
                break;
            }
            if(parent instanceof HorizontalScrollView){
                setHScrollViewMaxFlingVelocity((HorizontalScrollView) parent);
                break;
            }

            if(parent instanceof AbsListView){
                setAbsListViewMaxFlingVelocity((AbsListView) parent);
                break;
            }
            parent = (View) parent.getParent();
        }
    }


    private static void setRecycleViewMaxFlingVelocity(RecyclerView recyclerView){
        if(mMaxFlingVelocityForRecyclerView == 0){
            mMaxFlingVelocityForRecyclerView = recyclerView.getMaxFlingVelocity();
        }
        try{
            Field field = RecyclerView.class.getDeclaredField("mMaxFlingVelocity");
            field.setAccessible(true);
            int velocity = mMaxFlingVelocityForRecyclerView/2;
            if(velocity > 0){
                field.setInt(recyclerView, velocity);
            }
        }catch (Exception e){
            e.printStackTrace();
        }
    }

    private static void setScrollViewMaxFlingVelocity(ScrollView scrollView){
        try{
            Field field = ScrollView.class.getDeclaredField("mMaximumVelocity");
            field.setAccessible(true);
            if(mMaxFlingVelocityForScrollView == 0){
                mMaxFlingVelocityForScrollView = field.getInt(scrollView);
            }
            int velocity = mMaxFlingVelocityForScrollView /2;
            if(velocity > 0){
                field.setInt(scrollView, velocity);
            }
        }catch (Exception e){}
    }


    private static void setHScrollViewMaxFlingVelocity(HorizontalScrollView scrollView){
        try{
            Field field = HorizontalScrollView.class.getDeclaredField("mMaximumVelocity");
            field.setAccessible(true);
            if(mMaxFlingVelocityForHScrollView == 0){
                mMaxFlingVelocityForHScrollView = field.getInt(scrollView);
            }
            int velocity = mMaxFlingVelocityForHScrollView/2;
            if(velocity > 0){
                field.setInt(scrollView, velocity);
            }
        }catch (Exception e){}
    }
    private static void setAbsListViewMaxFlingVelocity(AbsListView listView) {
        try{
            Field field = AbsListView.class.getDeclaredField("mMaximumVelocity");
            field.setAccessible(true);
            if(mMaxFlingVelocityForAbsList == 0){
                mMaxFlingVelocityForAbsList = field.getInt(listView);
            }
            int velocity = mMaxFlingVelocityForAbsList/2;
            if(velocity > 0){
                field.setInt(listView, velocity);
            }
        }catch (Exception e){}
    }

    private static int mMaxFlingVelocityForRecyclerView = 0;
    private static int mMaxFlingVelocityForScrollView = 0;
    private static int mMaxFlingVelocityForHScrollView = 0;
    private static int mMaxFlingVelocityForAbsList = 0;
}

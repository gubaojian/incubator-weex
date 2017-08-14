<template>
    <div class="body">
      <list class="list">
         <refresh class="refresh-view" :display="refresh_display" @refresh="onrefresh">
         </refresh>
         <cell>
            <div class="cell"></div>
         </cell>
         <cell>
            <div class="cell"></div>
         </cell>

      </list>
    </div>

</template>

<style>
.refresh-view{
   height:80px;
   justify-content:center;
   align-items:center;
   background-color:red;
}

.cell{
    background-color:red;
    height: 176px;
}
.body{
  background-color:yellow;
}

</style>

<script>
module.exports = {
    data: function() {
      return {
         refresh_display: 'hide',
      }
    },
    methods:{
      onScroll: function(e){

      },
      onrefresh : function(e){
        var self = this;
        this.refresh_display = "show";
         setTimeout(function () {
          self.refresh_display = 'hide';
        }, 1000)
      }
    }
  }
</script>

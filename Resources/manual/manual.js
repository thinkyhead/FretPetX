$(function(){
  
  var top = $('#sidebar').offset().top - parseFloat($('#sidebar').css('marginTop').replace(/auto/, 0));
  var kk = $('#sidebar .keykey');
  var kktop = kk.offset().top - parseFloat(kk.css('marginTop').replace(/auto/, 0)) - top;
  kk.css('top',kktop+'px');

  $(window).scroll(function (event) {
    // what the y position of the scroll is
    var y = $(this).scrollTop();
  
    // whether that's below the form
    if (y >= top) {
      // if so, ad the fixed class
      $('#sidebar').addClass('fixed');
    } else {
      // otherwise remove it
      $('#sidebar').removeClass('fixed');
    }
  });
  
});
class Brunch {
  init(food, drink) {
    this.food=food;
    this.drink=drink;
  }
}

var b=Brunch("eggs", "coffee");
print b.food+"喝的"+b.drink;
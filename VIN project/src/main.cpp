#include <Arduino.h>
#include <DigitLedDisplay.h>
// display needs 3 digital pins, 2,3,4

#define displayDIN 2
#define displayCS 3
#define displayCLK 4

// DigitLedDisplay ld = DigitLedDisplay(displayDIN, displayCLK, displayCS);
DigitLedDisplay ld = DigitLedDisplay(displayDIN, displayCS, displayCLK);

//+-*/ enter and clear on analog 1-6 (as digital)

#define buttonPlus 15
#define buttonMinus 16
#define buttonMult 17
#define buttonDivide 18
#define buttonEnter 19
#define buttonClear 6

// 0,1,2,3,4,5,6,7,8,9 on digital 5-14
#define button0 5
#define button1 6
#define button2 7
#define button3 8
#define button4 9
#define button5 10
#define button6 11
#define button7 12
#define button9 14

//                    0       1      2      3     4       5     6       7       8       9     +     -       *       /    enter  clear
bool keyPressed[16] = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};

String calcString = "";
String numToDisplay = "";
void ClearScreen()
{
  numToDisplay = "";
  calcString = "";
  ld.on();
  ld.clear();
}

void DisplayError()
{
  Serial.println("Error");
  ld.write(1, B01111111);
  ld.write(2, B01111111);
  ld.write(3, B01111111);
  ld.write(4, B01111111);
}

char DisplayNumWithDot(char num)
{
  switch (num)
  {
  case '0':
    return B11111110;
  case '1':
    return B10110000;
  case '2':
    return B11101101;
  case '3':
    return B11111001;
  case '4':
    return B10110011;
  case '5':
    return B11011011;
  case '6':
    return B11011111;
  case '7':
    return B11110000;
  case '8':
    return B11111111;
  case '9':
    return B11111011;
  default:
    break;
  }
}

void DisplayNum()
{
  ld.on();
  ld.clear();
  if (numToDisplay.length() > 8)
    numToDisplay = numToDisplay.substring(0, 8);
  bool isDecimal = false;
  int index = numToDisplay.indexOf(".");
  if (index != -1)
    isDecimal = true;

  int dp = numToDisplay.substring(index + 1).toInt();
  if (dp == 0)
    isDecimal = false;
  // if is decimal
  if (isDecimal)
  {
    int decimalPart = 0;
    int wholePart = 0;
    decimalPart = numToDisplay.substring(index + 1).toInt();

    while (decimalPart > 99)
    {
      decimalPart = decimalPart / 10;
    }

    if (decimalPart < 10)
      ld.printDigit(0, 1);
    ld.printDigit(decimalPart, 0);
    wholePart = numToDisplay.substring(0, index).toInt();

    ld.printDigit(wholePart, 2);
    ld.write(3, B00000000);
    char lastChar = String(wholePart)[String(wholePart).length() - 1];
    ld.write(3, DisplayNumWithDot(lastChar));
  }
  else
  {
    ld.printDigit(numToDisplay.toInt());
  }
}

int CountOperators(String string)
{
  int count = 0;
  for (int i = 0; i < string.length(); i++)
  {
    if (string[i] == '+' || string[i] == '-' || string[i] == '*' || string[i] == '/')
      count++;
  }
  return count;
}
int HighestOperatorPriority(String string)
{
  for (int i = 0; i < string.length(); i++)
  {
    if (string[i] == '*' || string[i] == '/')
      return 2;
  }
  return 1;
}
int FindNextOperator(String string)
{
  for (int i = 0; i < string.length(); i++)
  {
    if (string[i] == '+' || string[i] == '-' || string[i] == '*' || string[i] == '/')
      return i;
  }
  return -1;
}
int FindNextOperator(String string, int from)
{
  for (int i = from; i < string.length(); i++)
  {
    if (string[i] == '+' || string[i] == '-' || string[i] == '*' || string[i] == '/')
      return i;
  }
  return -1;
}
int FindPrevOperator(String string, int from)
{
  for (int i = from; i < string.length(); i--)
  {
    if (string[i] == '+' || string[i] == '-' || string[i] == '*' || string[i] == '/')
      return i;
  }
  return -1;
}

String ProcessCalculation(String string)
{
  if (CountOperators(string) == 0)
    return string;

  while (CountOperators(string) > 0)
  {
    Serial.println("OpCount: " + String(CountOperators(string)) + "On string: " + string);

    if (HighestOperatorPriority(string) == 1)
    {
      // + -
      int nextOp = FindNextOperator(string);
      String tempString = string.substring(nextOp + 1);

      float num1 = string.substring(0, nextOp).toFloat();

      int nextNextOp = FindNextOperator(tempString);
      float num2;
      if (nextNextOp == -1)
      {
        num2 = tempString.substring(0).toFloat();
      }
      else
      {
        num2 = tempString.substring(0, nextNextOp).toFloat();
      };
      if (nextNextOp == -1)
      {
        Serial.println("n1: " + String(num1) + "n2: " + String(num2));
        string = (string[nextOp] == '+') ? String(num1 + num2) : String(num1 - num2);
      }
      else
      {
        string = (string[nextOp] == '+') ? String(num1 + num2) + tempString.substring(nextNextOp) : String(num1 - num2) + tempString.substring(nextNextOp);
      }
    }
    else
    {
      // * /

      // find first * or /
      int first;
      for (size_t i = 0; i < string.length(); i++)
      {
        if (string[i] == '*' || string[i] == '/')
        {
          first = i;
          break;
        }
      }
      // find next operator
      int nextOp = FindNextOperator(string, first + 1);
      // find prev operator
      int prevOp = FindPrevOperator(string, first - 1);

      // split string from begining to prev operator(including), and from next operator(including) to end
      String beforeString;
      if (prevOp == -1)
      {
        beforeString = "";
      }
      else
      {
        beforeString = string.substring(0, prevOp + 1);
      }
      String afterString;
      if (nextOp == -1)
      {
        afterString = "";
      }
      else
      {
        afterString = string.substring(nextOp);
      }
      float num1 = string.substring(prevOp + 1, first).toFloat();
      float num2 = string.substring(first + 1, nextOp).toFloat();
      Serial.println("n1: " + String(num1) + "n2: " + String(num2) + "first: " + String(first) + "before: " + beforeString + "after: " + afterString);
      if (string[first] == '*')
      {
        num1 = num1 * num2;
      }
      else
      {
        if (num2 == 0)
        {
          Serial.println("DBZ");
          return "DBZ";
        }

        num1 = num1 / num2;
      }
      string = beforeString + String(num1) + afterString;
    }
  }
  return string;
}

String pressKey(int key)
{
  if (keyPressed[key])
    return "";

  keyPressed[key] = true;
  String calc = "";
  switch (key)
  {
  case 8:
    calcString += String(0);
    numToDisplay += String(0);
    return "Pressed: " + String(0);
  case 12:
    calcString += String(1);
    numToDisplay += String(1);
    return "Pressed: " + String(1);
  case 11:
    calcString += String(2);
    numToDisplay += String(2);
    return "Pressed: " + String(2);
  case 10:
    calcString += String(3);
    numToDisplay += String(3);
    return "Pressed: " + String(3);
  case 4 ... 6:
    calcString += String(key);
    numToDisplay += String(key);
    return "Pressed: " + String(key);
  case 0:
    calcString += String(7);
    numToDisplay += String(7);
    return "Pressed: " + String(7);
  case 1:
    calcString += String(8);
    numToDisplay += String(8);
    return "Pressed: " + String(8);
  case 2:
    calcString += String(9);
    numToDisplay += String(9);
    return "Pressed: " + String(9);
  case 3:
    calcString += "+";
    numToDisplay = "";
    return "Pressed: +";
  case 7:
    calcString += "-";
    numToDisplay = "";
    return "Pressed: -";
  case 9:
    calcString += "*";
    numToDisplay = "";
    return "Pressed: *";
  case 13:
    calcString += "/";
    numToDisplay = "";
    return "Pressed: /";
  case 15:
    calcString = "";
    numToDisplay = "";
    ClearScreen();
    return "Cleared!";

  case 14:
    calc = ProcessCalculation(calcString);
    numToDisplay = calc.toFloat();
    calcString = calc;
    return (calc);
  default:
    break;
  }
}

void setup()
{
  ld.setBright(1);
  ld.setDigitLimit(8);

  Serial.begin(9600);
  for (int i = button0; i <= buttonEnter; i++)
  {
    pinMode(i, INPUT_PULLUP);
  }
  pinMode(A6, INPUT);
  pinMode(A7, INPUT);
  pinMode(8, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);

  pinMode(displayCLK, OUTPUT);
  pinMode(displayDIN, OUTPUT);
  pinMode(displayCS, OUTPUT);
}

void loop()
{
  for (int i = button0; i <= buttonEnter; i++)
  {
    if (i == 13)
      continue;
    if (digitalRead(i) == LOW)
    {
      String out = pressKey(i - 5);
      if (out == "DBZ")
        DisplayError();
      else
        DisplayNum();
      if (out != "")
        Serial.println(out);
    }
    else
    {
      keyPressed[i - 5] = false;
    }
  }

  if (analogRead(6) >= 1022)
  {
    Serial.println("PRESSED CLEAR");
    String out = pressKey(15);
    if (out != "")
      Serial.println(out);
  }
  else
    keyPressed[15] = false;

  if (analogRead(7) >= 512)
  {
    String out = pressKey(8);
    if (out == "DBZ")
      DisplayError();
    else
      DisplayNum();
    if (out != "")
      Serial.println(out);
  }
  else
  {
    keyPressed[8] = false;
  }

  delay(25);
}

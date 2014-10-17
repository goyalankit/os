#include <iostream>
#include <fstream>
using namespace std;

int main(){
  ofstream os("test.txt");
  os << "1. Hi." << endl <<"2. By mom!" << endl << endl << "3. YES I did it." << endl << "4. I'm fine." << endl << "5. I'm writing different lines to the file, OK?";

  os.close();

  //making a copy of whatever has been written in test.txt
  char line[100]; //assuming a line wouldn't exceed more than 100 characters.

  while (!EOF){
    ifstream is("test.txt");

    is.getline(line,100,'\n');
    is.close();

    os.open("copy_of_test.txt", ios::out);
    os << line << endl;
    os.close();
  }//Copy of test.txt has been created.

  ifstream is("copy_of_test.txt");

  while (is.tellg() != 0){ //loop continues untill everything from copy_of_text.txt has been removed. ***THIS LINE GIVES ERROR***
    seekg(0,ios::end);
    int sizeOfFile  = tellg();

    while ( seekg(rand()%sizeOfFile) != '\n' );
    seeg(1,ios::current);

    is.getline(line,100,'\n');

    cout << line << endl;

    //deleting that line.
    is.close(); //AND I DON't KNOW HOW TO DELETE THE LINE WHICH HAS BEEN Displayed once.

  }

  return 0;
}

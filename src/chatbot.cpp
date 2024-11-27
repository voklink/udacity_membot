#include <iostream>
#include <random>
#include <algorithm>
#include <ctime>

#include "chatlogic.h"
#include "graphnode.h"
#include "graphedge.h"
#include "chatbot.h"

// constructor WITHOUT memory allocation
ChatBot::ChatBot()
{
    std::cout << "ChatBot constructed: " << this << std::endl;
    // invalidate data handles
    _image = nullptr;
    _chatLogic = nullptr;
    _rootNode = nullptr;
}

// constructor WITH memory allocation
ChatBot::ChatBot(std::string filename)
{
    std::cout << "ChatBot constructed: " << this << std::endl;
    
    // invalidate data handles
    _chatLogic = nullptr;
    _rootNode = nullptr;

    // load image into heap memory
    _image = new wxBitmap(filename, wxBITMAP_TYPE_PNG);
}

ChatBot::~ChatBot()
{
    std::cout << "    ChatBot destroyed: " << this << std::endl;

    // deallocate heap memory
    if(_image != NULL) // Attention: wxWidgets used NULL and not nullptr
    {
        //delete _image;
    
        _image = NULL;
    }
}

//// STUDENT CODE
////
// Implementing the Rule of Five

// TODO:
// In the MOVE operations:
// Do you have to "delete" _image (target) first?? Like in the example 5.3?
// and then allocate fresh memory via NEW? Then you would have to actually move all the data
// instead of just passing on the ownership.
// Or does wxBitmap already its won RuleOfFive which allocates the correct memory? 

// Copy Constructor
// srcObject is const as it is not being changed
ChatBot::ChatBot(const ChatBot& srcObject)
{
    std::cout << "Chatbot copied (constr) from" << &srcObject << " to " << this  << "\n";
    
    // Allocating a NEW heap by directly copying the old image
    // Or can I copy this directly??? wxBitmap should have its own CopyConstructor/Assignment
    // in the construcor NEW is used (if an image is given...)
    // so maybe I have to do that here too!
    // _image = new wxBitmap(*srcObject._image);

    // Now copying each attribute/member one by one from the srcObj to our new obj.
    _image          = srcObject._image;
    _currentNode    = srcObject._currentNode;
    _rootNode       = srcObject._rootNode;
    _chatLogic      = srcObject._chatLogic;
    _chatLogic->SetChatbotHandle(this);
}

// Copy Assignment Operator
ChatBot& ChatBot::operator=(const ChatBot& srcObject)
{
    std::cout << "Chatbot copied (assign) from " << &srcObject << " to " << this << "\n";
    // Safety check, if object is copy-assigned to itself
    if (this == &srcObject)
    {
        return *this;
    }

    // Now, just like in the CopyConstructor, copy each variable one by one
    // _image          = new wxBitmap(*srcObject._image);
    _image          = srcObject._image;
    _currentNode    = srcObject._currentNode;
    _rootNode       = srcObject._rootNode;
    _chatLogic      = srcObject._chatLogic;
    _chatLogic->SetChatbotHandle(this);
    return *this;
}

// Move Constructor
// srcObject is not CONST as we reset it after the move
// so first, we copy, then we reset the srcObj
ChatBot::ChatBot(ChatBot&& srcObject)
{
    std::cout << "ChatBot moved (constr) from " << &srcObject << " to " << this << "\n";
    
    // _image          = new wxBitmap(*srcObject._image);
    _image          = srcObject._image;
    _currentNode    = srcObject._currentNode;
    _rootNode       = srcObject._rootNode;
    _chatLogic      = srcObject._chatLogic;
    _chatLogic->SetChatbotHandle(this);

    srcObject._image          = NULL;
    srcObject._currentNode    = nullptr;
    srcObject._rootNode       = nullptr;
    srcObject._chatLogic      = nullptr;
}

// Move Assignment Operator
// srcObject is not CONST as we reset it after the move
// so first, we copy, then we reset the srcObj
ChatBot& ChatBot::operator=(ChatBot&& srcObject)  
{
    std::cout << "Chatbot moved (assign) from " << &srcObject << " to " << this << "\n";
    // Safety check, if object is copy-assigned to itself
    if (this == &srcObject)
    {
        return *this;
    }
    
    // _image          = new wxBitmap(*srcObject._image);
    _image          = srcObject._image;
    _currentNode    = srcObject._currentNode;
    _rootNode       = srcObject._rootNode;
    _chatLogic      = srcObject._chatLogic;
    _chatLogic->SetChatbotHandle(this);

    srcObject._image          = NULL;
    srcObject._currentNode    = nullptr;
    srcObject._rootNode       = nullptr;
    srcObject._chatLogic      = nullptr;
    return *this;
}

////
//// EOF STUDENT CODE

void ChatBot::ReceiveMessageFromUser(std::string message)
{
    // loop over all edges and keywords and compute Levenshtein distance to query
    typedef std::pair<GraphEdge *, int> EdgeDist;
    std::vector<EdgeDist> levDists; // format is <ptr,levDist>

    for (size_t i = 0; i < _currentNode->GetNumberOfChildEdges(); ++i)
    {
        GraphEdge *edge = _currentNode->GetChildEdgeAtIndex(i);
        for (auto keyword : edge->GetKeywords())
        {
            EdgeDist ed{edge, ComputeLevenshteinDistance(keyword, message)};
            levDists.push_back(ed);
        }
    }

    // select best fitting edge to proceed along
    GraphNode *newNode;
    if (levDists.size() > 0)
    {
        // sort in ascending order of Levenshtein distance (best fit is at the top)
        std::sort(levDists.begin(), levDists.end(), [](const EdgeDist &a, const EdgeDist &b) { return a.second < b.second; });
        newNode = levDists.at(0).first->GetChildNode(); // after sorting the best edge is at first position
    }
    else
    {
        // go back to root node
        newNode = _rootNode;
    }

    // tell current node to move chatbot to new node
    _currentNode->MoveChatbotToNewNode(newNode);
}

void ChatBot::SetCurrentNode(GraphNode *node)
{
    // update pointer to current node
    _currentNode = node;

    // select a random node answer (if several answers should exist)
    std::vector<std::string> answers = _currentNode->GetAnswers();
    std::mt19937 generator(int(std::time(0)));
    std::uniform_int_distribution<int> dis(0, answers.size() - 1);
    std::string answer = answers.at(dis(generator));

    // send selected node answer to user
    _chatLogic->SendMessageToUser(answer);
}

int ChatBot::ComputeLevenshteinDistance(std::string s1, std::string s2)
{
    // convert both strings to upper-case before comparing
    std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);

    // compute Levenshtein distance measure between both strings
    const size_t m(s1.size());
    const size_t n(s2.size());

    if (m == 0)
        return n;
    if (n == 0)
        return m;

    size_t *costs = new size_t[n + 1];

    for (size_t k = 0; k <= n; k++)
        costs[k] = k;

    size_t i = 0;
    for (std::string::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1, ++i)
    {
        costs[0] = i + 1;
        size_t corner = i;

        size_t j = 0;
        for (std::string::const_iterator it2 = s2.begin(); it2 != s2.end(); ++it2, ++j)
        {
            size_t upper = costs[j + 1];
            if (*it1 == *it2)
            {
                costs[j + 1] = corner;
            }
            else
            {
                size_t t(upper < corner ? upper : corner);
                costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
            }

            corner = upper;
        }
    }

    int result = costs[n];
    delete[] costs;

    return result;
}
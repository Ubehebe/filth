#include "FindWork.hpp"

using namespace std;

void FindWork::clear_Workmap()
{
  /* The reason for this strange pattern is that the derived Work destructor
   * should remove itself from the statemap, which would invalidate our
   * iterator. */
  list<Work *> todel;
  for (Workmap::iterator it = st.begin(); it != st.end(); ++it)
    todel.push_back(it->second);
  for (list<Work *>::iterator it = todel.begin(); it != todel.end(); ++it)
    delete *it;
  st.clear();
}

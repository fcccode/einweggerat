#pragma once

class CInputView : public CDialogImpl<CInputView>
{
public:
   CListViewCtrl assign;
   CTreeViewCtrl event;
   CButton add;
   CButton remove;
   CEdit   editevent;
   input				   * input;
   dinput                 * di;
   bind_list              * bl;
   bool					 notify_d;
   bool                     have_event;
   dinput::di_event         last_event;
   std::vector< HTREEITEM > tree_items;
   guid_container         * guids;
   // returned to list view control
   enum { IDD = IDD_INPUT };

   BEGIN_MSG_MAP(CInputView)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialogView1)
      MESSAGE_HANDLER(WM_TIMER, OnTimer)
      MESSAGE_HANDLER(WM_DESTROY, OnClose)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      NOTIFY_HANDLER(IDC_LIST_ASSIGN, NM_DBLCLK, NotifyHandler)
      NOTIFY_HANDLER(IDC_LIST_ASSIGN, NM_CLICK, NotifyHandlerClick)
   END_MSG_MAP()

   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      KillTimer(0x1337);
      if (di)
      {
         delete di;
         di = 0;
      }
      return 0;
   }


   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      input->bl = bl->copy();
      Std_File_Writer_u out2;
      if (!out2.open(input->path))
      {
         input->save(out2);
         out2.close();
      }
      MessageBox(L"Settings saved", L"Input settings", MB_OK);
      return 0;
   }

   LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      KillTimer(0x1337);
      if (di)
      {
         delete di;
         di = 0;
      }
      bHandled = true;
      return 0;
   }

   LRESULT NotifyHandlerClick(int idCtrl, LPNMHDR pnmh, BOOL&/*bHandled*/) {
      editevent.SetWindowTextW(L"Double click on a entry to set bindings...");
      return 1;
   }

   LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL&/*bHandled*/) {
      //	KillTimer(0x1337);
      while (!process_events(di->read()))Sleep(10);
      std::tostringstream event_text;
      format_event(last_event, event_text);
      int n = ListView_GetSelectionMark(assign);
      if (n != -1)
      {
         dinput::di_event e;
         unsigned         action;
         unsigned retro_id;
         TCHAR description[64] = { 0 };
         bl->get(n, e, action, description, retro_id);
         add_to_list(assign, last_event, action, n, description, true);
         bl->replace(n, last_event, action, description, retro_id);

         input->bl = bl->copy();
         Std_File_Writer_u out2;
         if (!out2.open(input->path))
         {
            input->save(out2);
            out2.close();
            ListView_SetSelectionMark(assign, n);
            editevent.SetWindowTextW(L"Settings saved");
            return 1;
         }
         
      }
      ListView_SetSelectionMark(assign, n);
      editevent.SetWindowTextW(L"");
      //SetTimer(0x1337, 10);
      return 1;
   }


   LRESULT OnInitDialogView1(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      input = input::GetInstance();
      CLibretro* lib = CLibretro::GetInstance();
      bl = input->bl;
      guids = input->guids;
      di = create_dinput();
      if (di->open(input->lpDI, GetParent(), input->guids))
      {
         return 0;
      }


      assign = GetDlgItem(IDC_LIST_ASSIGN);
      event = GetDlgItem(IDC_TREE_ACTIONS);
      editevent = GetDlgItem(IDC_EDIT_EVENT);


      {
         LVCOLUMN lvc;
         memset(&lvc, 0, sizeof(lvc));

         lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
         lvc.fmt = LVCFMT_LEFT;
         lvc.cx = 200;
         lvc.pszText = _T("Event");
         lvc.iSubItem = 0;
         assign.InsertColumn(0, &lvc);

         lvc.cx = 150;
         lvc.pszText = _T("Core action");
         lvc.iSubItem = 1;

         assign.InsertColumn(1, &lvc);
         assign.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
         dinput::di_event e;
         unsigned         action;
         unsigned retro_id;

         for (unsigned i = 0, j = bl->get_count(); i < j; ++i)
         {
            TCHAR description[64] = { 0 };
            bl->get(i, e, action, description, retro_id);
            std::tostringstream event_text;
            format_event(e, event_text);
            add_to_list(assign, e, action, INT_MAX, description);
         }

      }
      editevent.SetFocus();
      SetTimer(0x1337, 10);
      notify_d = false;
      return 0;
   }

   LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      if (wParam == 0x1337)
      {

         if (process_events(di->read()))
         {
            std::tostringstream event_text;
            format_event(last_event, event_text);
            editevent.SetWindowTextW((LPCTSTR)event_text.str().c_str());
         }
      }

      return 0;
   }

private:


   void format_event(const dinput::di_event & e, std::tostringstream & out)
   {

      if (e.type == dinput::di_event::ev_key)
      {
         unsigned scancode = e.key.which;
         if (scancode == 0x45 || scancode == 0xC5) scancode ^= 0x80;
         scancode = ((scancode & 0x7F) << 16) | ((scancode & 0x80) << 17);
         TCHAR text[32];
         if (GetKeyNameText(scancode, text, 32))
            out << text;
      }
      else if (e.type == dinput::di_event::ev_none)
      {
         out << _T("None");
      }
      else if (e.type == dinput::di_event::ev_joy)
      {
         out << di->get_joystick_name(e.joy.serial);
         out << _T(' ');

         if (e.joy.type == dinput::di_event::joy_axis)
         {
            out << _T("axis ") << e.joy.which << _T(' ');
            if (e.joy.axis == dinput::di_event::axis_negative) out << _T('-');
            else out << _T('+');
         }
         else if (e.joy.type == dinput::di_event::joy_button)
         {
            out << _T("button ") << e.joy.which;
         }
         else if (e.joy.type == dinput::di_event::joy_pov)
         {
            out << _T("pov ") << e.joy.which << _T(" ") << e.joy.pov_angle << _T('�');
         }
      }
      else if (e.type == dinput::di_event::ev_xinput)
      {
         out << _T("Xbox360 controller #");
         out << e.xinput.index + 1;
         out << _T(' ');

         if (e.xinput.type == dinput::di_event::xinput_axis)
         {
            if (e.xinput.which & 2) out << _T("right");
            else out << _T("left");
            out << _T(" thumb stick ");
            if (e.xinput.which & 1)
            {
               if (e.xinput.axis == dinput::di_event::axis_positive) out << _T("up");
               else out << _T("down");
            }
            else
            {
               if (e.xinput.axis == dinput::di_event::axis_positive) out << _T("right");
               else out << _T("left");
            }
         }
         else if (e.xinput.type == dinput::di_event::xinput_trigger)
         {
            if (e.xinput.which == 0) out << _T("left");
            else out << _T("right");
            out << _T(" analog trigger");
         }
         else if (e.xinput.type == dinput::di_event::xinput_button)
         {
            static const TCHAR * xinput_button[] = { _T("D-pad up"), _T("D-pad down"), _T("D-pad left"), _T("D-pad right"), _T("Start button"), _T("Back button"), _T("left thumb stick button"), _T("right thumb stick button"), _T("left shoulder button"), _T("right shoulder button"), _T("A button"), _T("B button"), _T("X button"), _T("Y button") };
            out << xinput_button[e.xinput.which];
         }
      }
   }

   bool process_events(std::vector< dinput::di_event > events)
   {
      std::vector< dinput::di_event >::iterator it;
      bool     again;
      unsigned last = 0;
      do
      {
         again = false;
         for (it = events.begin() + last; it < events.end(); ++it)
         {
            if (it->type == dinput::di_event::ev_key)
            {
               if (it->key.type == dinput::di_event::key_up)
               {
                  again = true;
                  last = it - events.begin();
                  events.erase(it);
                  break;
               }
            }
            else if (it->type == dinput::di_event::ev_joy)
            {
               if (it->joy.type == dinput::di_event::joy_axis)
               {
                  if (it->joy.axis == dinput::di_event::axis_center)
                  {
                     again = true;
                     last = it - events.begin();
                     events.erase(it);
                     break;
                  }
               }
               else if (it->joy.type == dinput::di_event::joy_button)
               {
                  if (it->joy.button == dinput::di_event::button_up)
                  {
                     again = true;
                     last = it - events.begin();
                     events.erase(it);
                     break;
                  }
               }
               else if (it->joy.type == dinput::di_event::joy_pov)
               {
                  if (it->joy.pov_angle == ~0)
                  {
                     again = true;
                     last = it - events.begin();
                     events.erase(it);
                     break;
                  }
               }
            }
            else if (it->type == dinput::di_event::ev_xinput)
            {
               if (it->xinput.type == dinput::di_event::xinput_axis)
               {
                  if (it->xinput.axis == dinput::di_event::axis_center)
                  {
                     again = true;
                     last = it - events.begin();
                     events.erase(it);
                     break;
                  }
               }
               else if (it->xinput.type == dinput::di_event::xinput_trigger ||
                  it->xinput.type == dinput::di_event::xinput_button)
               {
                  if (it->xinput.button == dinput::di_event::button_up)
                  {
                     again = true;
                     last = it - events.begin();
                     events.erase(it);
                     break;
                  }
               }
            }
         }
      } while (again);

      it = events.end();
      if (it > events.begin())
      {
         --it;
         have_event = true;
         last_event = *it;
         return true;
      }

      return false;
   }

   void add_to_list(HWND w, const dinput::di_event & e, unsigned action, int item2, TCHAR* description, BOOL replace = false)
   {
      std::tstring str;
      {
         std::tostringstream text;
         format_event(e, text);
         str = text.str();
      }
      LVITEM lvi;
      memset(&lvi, 0, sizeof(lvi));
      lvi.mask = LVIF_TEXT;
      lvi.iItem = item2;
      lvi.pszText = (TCHAR *)str.c_str();
      if (!replace)
      {
         int item = ListView_InsertItem(w, &lvi);
         if (item != -1)
         {
            lvi.iSubItem = 1;
            lvi.pszText = (TCHAR *)description;
            SendMessage(w, LVM_SETITEMTEXT, (WPARAM)item, (LPARAM)(LVITEM *)& lvi);
         }
      }
      else
      {
         ListView_SetItem(w, &lvi);
      }

   }

   void remove_from_list(HWND w, unsigned n)
   {
      ListView_DeleteItem(w, n);
   }
};

#define PGS_EX_SINGLECLICKEDIT   0x00000001
#define PGS_EX_NOGRID            0x00000002
#define PGS_EX_TABNAVIGATION     0x00000004
#define PGS_EX_NOSHEETNAVIGATION 0x00000008
#define PGS_EX_FULLROWSELECT     0x00000010
#define PGS_EX_INVERTSELECTION   0x00000020
#define PGS_EX_ADDITEMATEND      0x00000040

namespace std
{
   typedef wstring        tstring;
   typedef wistringstream tistringstream;
   typedef wostringstream tostringstream;
}

std::wstring s2ws(const std::string& str)
{
   int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
   std::wstring wstrTo(size_needed, 0);
   MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
   return wstrTo;
}

std::string ws2s(const std::wstring &wstr)
{
   if (wstr.empty()) return std::string();
   int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
   std::string strTo(size_needed, 0);
   WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
   return strTo;
}

class CVariablesView : public CDialogImpl<CVariablesView>
{
public:
   HWND m_hwndOwner;
   enum { IDD = IDD_VARIABLES };
   BEGIN_MSG_MAP(CVariablesView)
      MESSAGE_HANDLER(WM_DESTROY, OnClose)
      MESSAGE_HANDLER(WM_CLOSE, OnClose)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialogView1)
      NOTIFY_CODE_HANDLER(PIN_SELCHANGED, OnSelChanged);
   NOTIFY_CODE_HANDLER(PIN_ITEMCHANGED, OnItemChanged);
   NOTIFY_CODE_HANDLER(PIN_ADDITEM, OnAddItem);
   REFLECT_NOTIFICATIONS()
   END_MSG_MAP()
   CPropertyGridCtrl m_grid;
   CLibretro *retro;

   LRESULT OnAddItem(int idCtrl, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      ATLTRACE(_T("OnAddItem - Ctrl: %d\n"), idCtrl); idCtrl;

      int i = m_grid.InsertItem(-1, PropCreateReadOnlyItem(_T(""), _T("Dolly")));
      m_grid.SetSubItem(i, 1, PropCreateSimple(_T(""), true));
      m_grid.SetSubItem(i, 2, PropCreateCheckButton(_T(""), false));
      m_grid.SetSubItem(i, 3, PropCreateSimple(_T(""), _T("")));
      m_grid.SelectItem(i);
      return 0;
   }

   LRESULT OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      return 0;
   }

   LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMPROPERTYITEM pnpi = (LPNMPROPERTYITEM)pnmh;
      int type = pnpi->prop->GetKind();
      LPCTSTR name = pnpi->prop->GetName();
      if (type == 4)
      {
         TCHAR szValue[100] = { 0 };
         pnpi->prop->GetDisplayValue(szValue, sizeof(szValue) / sizeof(TCHAR));
         CComVariant vValue;
         pnpi->prop->GetValue(&vValue);
         vValue.ChangeType(VT_BSTR);
         ATLTRACE(_T("OnItemChanged - Ctrl: %d, Name: '%s', DispValue: '%s', Value: '%ls'\n"),
            idCtrl, pnpi->prop->GetName(), szValue, vValue.bstrVal); idCtrl;

         for (int i = 0; i < retro->variables.size(); i++)
         {
            wstring str = s2ws(retro->variables[i].name);
            if (lstrcmpW(str.c_str(), (LPCTSTR)name) == 0)
            {
               wstring var = szValue;
               string var2 = ws2s(var);
               retro->variables[i].var = var2;
            }

         }
      }
      if (type == 6)
      {
         CComVariant vValue;
         pnpi->prop->GetValue(&vValue);
         vValue.ChangeType(VT_BOOL);
         for (int i = 0; i < retro->variables.size(); i++)
         {
            wstring str = s2ws(retro->variables[i].name);
            wstring str2 = (LPCTSTR)name;
            if (str== str2)
            {
               const char* check = vValue.boolVal ? "enabled" : "disabled";
               retro->variables[i].var=check;
            }

         }
      }
      retro->variables_changed = true;
      return 0;
   }


   LRESULT OnInitDialogView1(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      retro = CLibretro::GetInstance();
      m_grid.SubclassWindow(GetDlgItem(IDC_LIST_VARIABLES));
      m_grid.InsertColumn(0, _T("Option"), LVCFMT_LEFT, 200, 0);
      m_grid.InsertColumn(1, _T("Setting"), LVCFMT_LEFT, 150, 0);
      m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT);

      for (int i = 0; i < retro->variables.size(); i++)
      {
         string descript = retro->variables[i].description;
         wstring descript_w = s2ws(descript);
         string usedv = retro->variables[i].usevars;
         string vars = retro->variables[i].var;
         wstring varname = s2ws(retro->variables[i].name);
         m_grid.InsertItem(i, PropCreateReadOnlyItem(_T(""), descript_w.c_str()));
         if (strcmp(usedv.c_str(), "enabled|disabled") == 0 || strcmp(usedv.c_str(), "disabled|enabled") == 0)
         {
            bool check = strstr((char*)vars.c_str(), "enabled");
            m_grid.SetSubItem(i, 1, PropCreateCheckButton(varname.c_str(), check));
         }
         else
         {

            vector <wstring> options;
            options.clear();
            char *pch = (char*)retro->variables[i].usevars.c_str();
            while (pch != NULL)
            {
               char val[255] = { 0 };
               char* str2 = strstr(pch, (char*)"|");
               if (!str2)
               {
                  strcpy(val, pch);
                  break;
               }
               strncpy(val, pch, str2 - pch);
               std::wostringstream temp;
               temp << val;
               options.push_back(temp.str());
               pch += str2 - pch++;

            }
            std::wostringstream temp;
            temp << pch;
            options.push_back(temp.str());

            LPWSTR **wszArray = new LPWSTR*[options.size() + 1];
            int j = 0;
            for (j; j < options.size(); j++) {
               wszArray[j] = new LPWSTR[options[j].length()];
               lstrcpy((LPWSTR)wszArray[j], options[j].c_str());

            }
            wszArray[j] = NULL;


            m_grid.SetSubItem(i, 1, PropCreateList(varname.c_str(), (LPCTSTR*)wszArray));
            HPROPERTY hDisabled = m_grid.GetProperty(i, 1);
            TCHAR szValue[100] = { 0 };
            hDisabled->GetDisplayValue(szValue, sizeof(szValue) / sizeof(TCHAR));
            wstring variant = s2ws(retro->variables[i].var);
            CComVariant vValue(variant.c_str());
            vValue.ChangeType(VT_BSTR);
            m_grid.SetItemValue(hDisabled, &vValue);

            for (j = 0; j < options.size(); j++) {
               free(wszArray[j]);
            }
            free(wszArray);
         }
      }

      return TRUE;
   }

   void save()
   {
      FILE *fp = NULL;
      ini_t* ini = ini_create(NULL);
      for (int i = 0; i < retro->variables.size(); i++) {
         ini_property_add(ini, INI_GLOBAL_SECTION, retro->variables[i].name.c_str(), strlen(retro->variables[i].name.c_str()), retro->variables[i].var.c_str(),
            strlen(retro->variables[i].var.c_str()));
      }
      int size = ini_save(ini, NULL, 0); // Find the size needed
      char* data = (char*)malloc(size);
      size = ini_save(ini, data, size); // Actually save the file	
      ini_destroy(ini);
      fp = _wfopen(retro->core_config, L"w");
      fwrite(data, 1, size, fp);
      fclose(fp);
      free(data);
   }

   LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      save();
      return 0;
   }
};

class COptions : public CDialogImpl<COptions>
{
public:
   enum { IDD = IDD_OPTIONS };
   CDialogTabCtrl m_ctrlTab;
   CVariablesView m_view1;
   CInputView m_view2;

   BEGIN_MSG_MAP(COptions)
      MESSAGE_HANDLER(WM_CLOSE,OnClose)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(ID_SAVE, OnOK)
      COMMAND_ID_HANDLER(ID_CANCEL, OnCloseCmd)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   // Handler prototypes (uncomment arguments if needed):
   //	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   //	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   //	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CImageList imglist;
      CIcon coreoptions, inputoptions;
      inputoptions.LoadIcon(IDI_ICON2);
      coreoptions.LoadIcon(IDI_ICON3);
      imglist.Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 1);

      imglist.AddIcon(coreoptions);
      imglist.AddIcon(inputoptions);
      m_ctrlTab.SubclassWindow(GetDlgItem(IDC_TAB));
      m_view1.Create(m_hWnd);
      m_view2.Create(m_hWnd);
      m_ctrlTab.SetImageList(imglist.m_hImageList);
      TCITEM tci = { 0 };
      tci.mask = TCIF_TEXT | TCIF_IMAGE;
      tci.pszText = _T("libretro core options");
      tci.iImage = 0;
      m_ctrlTab.InsertItem(0, &tci, m_view1);
      tci.mask = TCIF_TEXT | TCIF_IMAGE;
      tci.pszText = _T("Input configuration");
      tci.iImage = 1;
      m_ctrlTab.InsertItem(1, &tci, m_view2);
      m_ctrlTab.SetCurSel(0);

      CenterWindow(GetParent());
      return TRUE;
   }

   LRESULT OnClose(UINT /*uMsg*/, WPARAM wID, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return TRUE;
   }

   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      // TODO: Add validation code 
      EndDialog(wID);
      return 0;
   }

   LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
};

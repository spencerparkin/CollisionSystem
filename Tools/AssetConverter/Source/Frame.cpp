#include "Frame.h"
#include "Canvas.h"
#include "App.h"
#include "GamePreview.h"
#include "Converter.h"
#include "Log.h"
#include "LogWindow.h"
#include "RenderObjectList.h"
#include "RenderObjectProperties.h"
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/aboutdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/choicdlg.h>
#include <wx/splitter.h>
#include <wx/panel.h>

Frame::Frame(const wxPoint& pos, const wxSize& size) : wxFrame(nullptr, wxID_ANY, "Imzadi Asset Converter", pos, size), timer(this, ID_Timer)
{
	wxMenu* fileMenu = new wxMenu();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_ConvertAsset, "Convert Asset...", "Convert the output of some artsy-fartsy 3D modeling program into assets consumable as input to the Imzadi Game Engine."));
	fileMenu->Append(new wxMenuItem(fileMenu, ID_PreviewAsset, "Preview Asset...", "Select one or more Imzadi asset files to load and preview in engine."));
	fileMenu->AppendSeparator();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_ClearScene, "Clear Scene", "Remove all render objects from the current scene."));
	fileMenu->AppendSeparator();
	fileMenu->Append(new wxMenuItem(fileMenu, ID_Exit, "Exit", "Go skiing."));

	wxMenu* optionsMenu = new wxMenu();
	optionsMenu->Append(new wxMenuItem(optionsMenu, ID_ShowLogWindow, "Show Log Window", "Remove or bring back a window showing logging output.", wxITEM_CHECK));
	optionsMenu->Append(new wxMenuItem(optionsMenu, ID_ShowSkeleton, "Show Skeleton", "Render line-segments to illustrate the skeleton of skinned meshes being previews.", wxITEM_CHECK));

	wxMenu* helpMenu = new wxMenu();
	helpMenu->Append(new wxMenuItem(helpMenu, ID_About, "About", "Show the about-box."));

	wxMenuBar* menuBar = new wxMenuBar();
	menuBar->Append(fileMenu, "File");
	menuBar->Append(optionsMenu, "Options");
	menuBar->Append(helpMenu, "Help");
	this->SetMenuBar(menuBar);

	this->CreateStatusBar();

	this->Bind(wxEVT_MENU, &Frame::OnConvertAsset, this, ID_ConvertAsset);
	this->Bind(wxEVT_MENU, &Frame::OnPreviewAsset, this, ID_PreviewAsset);
	this->Bind(wxEVT_MENU, &Frame::OnClearScene, this, ID_ClearScene);
	this->Bind(wxEVT_MENU, &Frame::OnExit, this, ID_Exit);
	this->Bind(wxEVT_MENU, &Frame::OnAbout, this, ID_About);
	this->Bind(wxEVT_MENU, &Frame::OnShowLogWindow, this, ID_ShowLogWindow);
	this->Bind(wxEVT_MENU, &Frame::OnShowSkeleton, this, ID_ShowSkeleton);
	this->Bind(wxEVT_TIMER, &Frame::OnTimer, this, ID_Timer);
	this->Bind(wxEVT_UPDATE_UI, &Frame::OnUpdateUI, this, ID_ShowLogWindow);
	this->Bind(wxEVT_UPDATE_UI, &Frame::OnUpdateUI, this, ID_ShowSkeleton);
	this->Bind(wxEVT_CLOSE_WINDOW, &Frame::OnCloseWindow, this);

	wxSplitterWindow* mainSplitter = new wxSplitterWindow(this, wxID_ANY);
	mainSplitter->SetMinimumPaneSize(100);

	this->canvas = new Canvas(mainSplitter);

	wxPanel* sidePanel = new wxPanel(mainSplitter);
	wxSplitterWindow* sideSplitter = new wxSplitterWindow(sidePanel, wxID_ANY);

	this->renderObjectList = new RenderObjectList(sideSplitter);
	this->renderObjectProperties = new RenderObjectProperties(sideSplitter);

	wxBoxSizer* sideSizer = new wxBoxSizer(wxVERTICAL);
	sideSizer->Add(sideSplitter, 1, wxALL | wxEXPAND, 0);
	sidePanel->SetSizer(sideSizer);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
	mainSizer->Add(mainSplitter, 1, wxALL | wxEXPAND, 0);
	this->SetSizer(mainSizer);

	mainSplitter->SplitVertically(this->canvas, sidePanel);
	mainSplitter->SetSashPosition(800);

	sideSplitter->SplitHorizontally(this->renderObjectList, this->renderObjectProperties);
	sideSplitter->SetSashPosition(200);

	this->inTimer = false;
	this->timer.Start(0);
}

/*virtual*/ Frame::~Frame()
{
}

void Frame::OnTimer(wxTimerEvent& event)
{
	if (this->inTimer)
		return;

	this->inTimer = true;

	Imzadi::Game::Get()->Run();

	this->inTimer = false;
}

void Frame::OnShowLogWindow(wxCommandEvent& event)
{
	if (!Imzadi::LoggingSystem::Get()->RouteExists("log_window"))
		Imzadi::LoggingSystem::Get()->AddRoute(new LogWindowRoute());
	else
		Imzadi::LoggingSystem::Get()->RemoveRoute("log_window");
}

void Frame::OnShowSkeleton(wxCommandEvent& event)
{
	Imzadi::AnimatedMeshInstance::SetRenderSkeletons(!Imzadi::AnimatedMeshInstance::GetRenderSkeletons());
}

void Frame::OnUpdateUI(wxUpdateUIEvent& event)
{
	switch (event.GetId())
	{
		case ID_ShowLogWindow:
		{
			event.Check(Imzadi::LoggingSystem::Get()->RouteExists("log_window"));
			break;
		}
		case ID_ShowSkeleton:
		{
			event.Check(Imzadi::AnimatedMeshInstance::GetRenderSkeletons());
			break;
		}
	}
}

void Frame::OnConvertAsset(wxCommandEvent& event)
{
	wxFileDialog fileDialog(this, "Choose file(s) to convert.", wxEmptyString, wxEmptyString, "Any file (*.*)|*.*", wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
	if (fileDialog.ShowModal() != wxID_OK)
		return;
	
	wxArrayString choiceArray;
	choiceArray.Add("Meshes");
	choiceArray.Add("Animations");
	wxMultiChoiceDialog choiceDialog(this, "Export what?", "What to Export", choiceArray);
	if (choiceDialog.ShowModal() != wxID_OK)
		return;

	uint32_t flags = 0;
	wxArrayInt selectionArray = choiceDialog.GetSelections();
	for (int i = 0; i < selectionArray.size(); i++)
	{
		const wxString& selection = choiceArray[selectionArray[i]];
		if (selection == "Meshes")
			flags |= Converter::Flag::CONVERT_MESHES;
		else if (selection == "Animations")
			flags |= Converter::Flag::CONVERT_ANIMATIONS;
	}

	// TODO: Shouldn't hard-code this path.
	Converter converter(R"(E:\ENG_DEV\Imzadi\Games\SearchForTheSacredChaliceOfRixx\Assets)");

	wxArrayString fileArray;
	fileDialog.GetPaths(fileArray);
	for (const wxString& file : fileArray)
		converter.Convert(file, flags);
}

void Frame::OnPreviewAsset(wxCommandEvent& event)
{
	wxFileDialog fileDialog(this, "Choose file(s) to preview.", wxEmptyString, wxEmptyString, "Any file (*.*)|*.*", wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST);
	if (fileDialog.ShowModal() != wxID_OK)
		return;

	Imzadi::Game* game = Imzadi::Game::Get();
	wxString errorMsg;

	wxArrayString fileArray;
	fileDialog.GetPaths(fileArray);
	for (const wxString& file : fileArray)
	{
		Imzadi::Reference<Imzadi::RenderObject> renderObject = game->LoadAndPlaceRenderMesh((const char*)file.c_str(), Imzadi::Vector3(0.0, 0.0, 0.0), Imzadi::Quaternion());
		if (!renderObject)
			errorMsg += wxString::Format("Failed to load asset: %s\n", file.c_str());
		else
			this->renderObjectList->AddRenderObject(renderObject.Get());
	}

	if (errorMsg.length() > 0)
		wxMessageBox(errorMsg, "Error!", wxICON_ERROR | wxOK, this);

	this->renderObjectList->UpdateListView();
}

void Frame::OnClearScene(wxCommandEvent& event)
{
	this->renderObjectList->Clear();
	this->renderObjectList->UpdateListView();
	this->renderObjectProperties->Clear();

	Imzadi::Game* game = Imzadi::Game::Get();
	game->GetScene()->Clear();
	game->GetScene()->AddRenderObject(game->GetDebugLines());
}

void Frame::OnAbout(wxCommandEvent& event)
{
	wxAboutDialogInfo aboutDialogInfo;

	aboutDialogInfo.SetName("Imzadi Asset Converter");
	aboutDialogInfo.SetDescription("This program is designed to convert art program files into assets consumable by the Imzadi Game Engine.  It also provides a preview of the converted assets.");

	wxAboutBox(aboutDialogInfo);
}

void Frame::OnCloseWindow(wxCloseEvent& event)
{
	this->renderObjectList->Clear();

	wxFrame::OnCloseWindow(event);
}

void Frame::OnExit(wxCommandEvent& event)
{
	this->Close();
}
import SwiftUI

enum AppTab {
    case home, groups, files, account
}

struct AppLayoutView : View {
    @State private var selectedTab : AppTab = .home


    var body: some View {
        NavigationStack {
            VStack(spacing: 0) {
                TabView(selection: $selectedTab) {
                    HomeView()
                        .tag(AppTab.home)
                        .tabItem {
                            Label("", systemImage: selectedTab == .home ?
                                  "house.fill" : "house")
                        }
                    
                    GroupsView()
                        .tag(AppTab.groups)
                        .tabItem {
                            Label("", systemImage: selectedTab == .groups ? "person.3.fill" : "person.3")
                        }
                    
                    MyFilesView()
                        .tag(AppTab.files)
                        .tabItem {
                            Label("", systemImage: selectedTab == .files ?
                                  "folder.fill" : "folder")
                        }
                    
                    
                    
                    AccountView()
                        .tag(AppTab.account)
                        .tabItem {
                            Label("", systemImage: selectedTab == .account ?
                                  "person.fill" : "person")
                        }
                    
                }
            }
        }
        
    }

    func tabTitle(for tab: AppTab) -> String {
        switch tab {
        case .home: return "Home"
        case .files: return "My Files"
        case .account: return "Account"
        case .groups: return "Groups"
        }
    }
}   

#Preview {
    AppLayoutView()
}
